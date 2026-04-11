#include "auth.h"
#include <crypt.h>
#include <pwd.h>
#include <shadow.h>

static int initialized = 0;
static pthread_rwlock_t auth_lock = PTHREAD_RWLOCK_INITIALIZER;

typedef struct {
    char username[256];
    unsigned char password_hash[64];
    unsigned char salt[32];
    int hash_type;
} auth_cache_entry_t;

#define AUTH_CACHE_SIZE 1024
static auth_cache_entry_t auth_cache[AUTH_CACHE_SIZE];
static size_t auth_cache_count = 0;

int auth_init(const char *config_file) {
    pthread_rwlock_init(&auth_lock, NULL);
    memset(auth_cache, 0, sizeof(auth_cache));
    auth_cache_count = 0;
    initialized = 1;
    return 0;
}

void hash_sha256(const unsigned char *data, size_t data_len, unsigned char *hash) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, data_len);
    SHA256_Final(hash, &ctx);
}

void hash_pbkdf2(const char *password, const unsigned char *salt, 
                 unsigned int rounds, unsigned char *output, size_t output_len) {
    PKCS5_PBKDF2_HMAC(password, strlen(password), (const unsigned char *)salt, 
                      32, rounds, EVP_sha256(), output_len, output);
}

int auth_verify_password(const char *username, const char *password) {
    FILE *auth_file;
    char line[1024];
    char stored_user[256];
    char stored_hash[512];
    char stored_salt[64];
    int hash_type = 0;
    unsigned char computed_hash[64];
    unsigned char salt_bin[32];
    size_t i;
    
    if (!initialized) return -1;
    
    pthread_rwlock_rdlock(&auth_lock);
    
    auth_file = fopen("/etc/nexus-mp/passwd", "r");
    if (!auth_file) {
        pthread_rwlock_unlock(&auth_lock);
        return -1;
    }
    
    while (fgets(line, sizeof(line), auth_file)) {
        if (sscanf(line, "%255[^:]:%511[^:]:%63s:%d", 
                   stored_user, stored_hash, stored_salt, &hash_type) >= 3) {
            if (strcmp(stored_user, username) == 0) {
                for (i = 0; i < 32 && i * 2 < strlen(stored_salt); i++) {
                    sscanf(&stored_salt[i * 2], "%2hhx", &salt_bin[i]);
                }
                
                if (hash_type == 1) {
                    hash_pbkdf2(password, salt_bin, 100000, computed_hash, 64);
                    
                    char computed_hex[129];
                    for (i = 0; i < 64; i++) {
                        sprintf(&computed_hex[i * 2], "%02x", computed_hash[i]);
                    }
                    
                    fclose(auth_file);
                    pthread_rwlock_unlock(&auth_lock);
                    
                    if (strcmp(computed_hex, stored_hash) == 0) {
                        return 0;
                    }
                    return -1;
                }
            }
        }
    }
    
    fclose(auth_file);
    pthread_rwlock_unlock(&auth_lock);
    return -1;
}

int auth_get_user_info(const char *username, auth_user_t *user) {
    FILE *user_file;
    char line[1024];
    char stored_user[256];
    char stored_home[1024];
    struct passwd *pwd;
    
    if (!initialized || !user) return -1;
    
    pthread_rwlock_rdlock(&auth_lock);
    
    user_file = fopen("/etc/nexus-mp/users", "r");
    if (!user_file) {
        pwd = getpwnam(username);
        if (!pwd) {
            pthread_rwlock_unlock(&auth_lock);
            return -1;
        }
        
        strncpy(user->username, username, sizeof(user->username) - 1);
        strncpy(user->home, pwd->pw_dir, sizeof(user->home) - 1);
        user->uid = pwd->pw_uid;
        user->gid = pwd->pw_gid;
        snprintf(user->maildir, sizeof(user->maildir), "%s/.maildir", pwd->pw_dir);
        
        pthread_rwlock_unlock(&auth_lock);
        return 0;
    }
    
    while (fgets(line, sizeof(line), user_file)) {
        if (sscanf(line, "%255[^:]:%1023s", stored_user, stored_home) == 2) {
            if (strcmp(stored_user, username) == 0) {
                strncpy(user->username, username, sizeof(user->username) - 1);
                strncpy(user->home, stored_home, sizeof(user->home) - 1);
                snprintf(user->maildir, sizeof(user->maildir), "%s/.maildir", stored_home);
                user->uid = getuid();
                user->gid = getgid();
                
                fclose(user_file);
                pthread_rwlock_unlock(&auth_lock);
                return 0;
            }
        }
    }
    
    fclose(user_file);
    pthread_rwlock_unlock(&auth_lock);
    return -1;
}

int auth_check_permission(const char *username, const char *resource) {
    char resource_path[1024];
    struct stat st;
    
    snprintf(resource_path, sizeof(resource_path), "/var/mail/%s/%s", username, resource);
    
    if (stat(resource_path, &st) < 0) {
        return -1;
    }
    
    if (access(resource_path, R_OK | W_OK) == 0) {
        return 0;
    }
    
    return -1;
}

int auth_create_session(const char *username) {
    FILE *session_file;
    char session_path[1024];
    time_t session_time;
    char session_id[64];
    unsigned char rand_bytes[32];
    size_t i;
    
    time(&session_time);
    
    RAND_bytes(rand_bytes, 32);
    for (i = 0; i < 32; i++) {
        sprintf(&session_id[i * 2], "%02x", rand_bytes[i]);
    }
    
    snprintf(session_path, sizeof(session_path), "/tmp/.nexus-mp-session-%s-%d", username, getpid());
    
    session_file = fopen(session_path, "w");
    if (!session_file) {
        return -1;
    }
    
    fprintf(session_file, "%s:%ld:%u\n", session_id, session_time, getpid());
    fclose(session_file);
    
    chmod(session_path, 0600);
    
    return 0;
}

int auth_destroy_session(const char *username) {
    char session_path[1024];
    
    snprintf(session_path, sizeof(session_path), "/tmp/.nexus-mp-session-%s-%d", username, getpid());
    
    if (unlink(session_path) < 0 && errno != ENOENT) {
        return -1;
    }
    
    return 0;
}

char *hash_password(const char *password) {
    static char hash[513];
    unsigned char salt[32];
    unsigned char computed_hash[64];
    char salt_hex[65];
    size_t i;
    
    if (RAND_bytes(salt, 32) != 1) {
        return NULL;
    }
    
    for (i = 0; i < 32; i++) {
        sprintf(&salt_hex[i * 2], "%02x", salt[i]);
    }
    
    hash_pbkdf2(password, salt, 100000, computed_hash, 64);
    
    for (i = 0; i < 64; i++) {
        sprintf(&hash[i * 2], "%02x", computed_hash[i]);
    }
    
    return hash;
}

int verify_password_hash(const char *password, const char *hash) {
    unsigned char computed_hash[64];
    unsigned char salt[32];
    char salt_hex[65];
    const char *salt_ptr;
    size_t i;
    char computed_hex[129];
    
    salt_ptr = strchr(hash, ':');
    if (!salt_ptr) return -1;
    salt_ptr++;
    
    strncpy(salt_hex, salt_ptr, 64);
    salt_hex[64] = '\0';
    
    for (i = 0; i < 32 && i * 2 < strlen(salt_hex); i++) {
        sscanf(&salt_hex[i * 2], "%2hhx", &salt[i]);
    }
    
    hash_pbkdf2(password, salt, 100000, computed_hash, 64);
    
    for (i = 0; i < 64; i++) {
        sprintf(&computed_hex[i * 2], "%02x", computed_hash[i]);
    }
    
    if (strncmp(computed_hex, hash, 128) == 0) {
        return 0;
    }
    
    return -1;
}

int auth_verify_cram_md5(const char *username, const char *challenge, const char *response, user_t *user) {
    auth_user_t auth_user;
    unsigned char hash[16];
    char computed_response[64];
    size_t i;
    
    if (auth_get_user_info(username, &auth_user) < 0) {
        return -1;
    }
    
    for (i = 0; i < 16; i++) {
        sprintf(&computed_response[i * 2], "%02x", hash[i]);
    }
    
    if (strcmp(computed_response, response) == 0) {
        strncpy(user->username, username, sizeof(user->username) - 1);
        return 0;
    }
    
    return -1;
}

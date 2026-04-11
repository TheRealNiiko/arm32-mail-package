#include "auth.h"
#include <crypt.h>

static int initialized = 0;

int auth_init(const char *config_file) {
    initialized = 1;
    return 0;
}

int auth_verify_password(const char *username, const char *password) {
    FILE *auth_file;
    char line[512];
    char stored_user[128];
    char stored_hash[256];
    char *result;
    
    auth_file = fopen("/etc/nexus-mp/passwd", "r");
    if (!auth_file) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), auth_file)) {
        if (sscanf(line, "%127[^:]:%255s", stored_user, stored_hash) == 2) {
            if (strcmp(stored_user, username) == 0) {
                result = crypt(password, stored_hash);
                fclose(auth_file);
                
                if (result && strcmp(result, stored_hash) == 0) {
                    return 0;
                }
                return -1;
            }
        }
    }
    
    fclose(auth_file);
    return -1;
}

int auth_get_user_info(const char *username, auth_user_t *user) {
    FILE *auth_file;
    char line[512];
    char stored_user[128];
    char stored_hash[256];
    char stored_home[512];
    
    auth_file = fopen("/etc/nexus-mp/users", "r");
    if (!auth_file) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), auth_file)) {
        if (sscanf(line, "%127[^:]:%255[^:]:%511s", stored_user, stored_hash, stored_home) == 3) {
            if (strcmp(stored_user, username) == 0) {
                strncpy(user->username, username, sizeof(user->username) - 1);
                strncpy(user->password_hash, stored_hash, sizeof(user->password_hash) - 1);
                strncpy(user->home, stored_home, sizeof(user->home) - 1);
                snprintf(user->maildir, sizeof(user->maildir), "%s/.maildir", stored_home);
                user->uid = getuid();
                user->gid = getgid();
                
                fclose(auth_file);
                return 0;
            }
        }
    }
    
    fclose(auth_file);
    return -1;
}

int auth_check_permission(const char *username, const char *resource) {
    char resource_path[512];
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
    char session_path[512];
    time_t session_time;
    
    time(&session_time);
    snprintf(session_path, sizeof(session_path), "/tmp/.nexus-mp-session-%s-%d", username, getpid());
    
    session_file = fopen(session_path, "w");
    if (!session_file) {
        return -1;
    }
    
    fprintf(session_file, "%ld\n", session_time);
    fclose(session_file);
    
    return 0;
}

int auth_destroy_session(const char *username) {
    char session_path[512];
    
    snprintf(session_path, sizeof(session_path), "/tmp/.nexus-mp-session-%s-%d", username, getpid());
    
    if (unlink(session_path) < 0 && errno != ENOENT) {
        return -1;
    }
    
    return 0;
}

char *hash_password(const char *password) {
    static char hash[256];
    char salt[16];
    int i;
    FILE *urandom;
    unsigned char random_bytes[8];
    char *result;
    
    urandom = fopen("/dev/urandom", "r");
    if (!urandom) {
        return NULL;
    }
    
    if (fread(random_bytes, 1, 8, urandom) != 8) {
        fclose(urandom);
        return NULL;
    }
    
    fclose(urandom);
    
    snprintf(salt, sizeof(salt), "$6$");
    for (i = 0; i < 8; i++) {
        snprintf(salt + strlen(salt), sizeof(salt) - strlen(salt), "%02x", random_bytes[i]);
    }
    
    result = crypt(password, salt);
    if (result) {
        strncpy(hash, result, sizeof(hash) - 1);
        return hash;
    }
    
    return NULL;
}

int verify_password_hash(const char *password, const char *hash) {
    char *result;
    
    result = crypt(password, hash);
    if (result && strcmp(result, hash) == 0) {
        return 0;
    }
    
    return -1;
}

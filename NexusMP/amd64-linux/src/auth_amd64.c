#include "auth_amd64.h"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <string.h>
#include <stdio.h>

auth_database_amd64_t *auth_db_create_amd64(void) {
    auth_database_amd64_t *db = malloc(sizeof(auth_database_amd64_t));
    if (!db) return NULL;

    db->users = malloc(sizeof(auth_user_amd64_t) * 16);
    if (!db->users) {
        free(db);
        return NULL;
    }

    db->user_count = 0;
    db->user_capacity = 16;
    pthread_rwlock_init(&db->lock, NULL);

    return db;
}

void auth_db_free_amd64(auth_database_amd64_t *db) {
    if (!db) return;
    
    pthread_rwlock_destroy(&db->lock);
    if (db->users) {
        free(db->users);
    }
    free(db);
}

void auth_generate_salt_amd64(uint8_t *salt, int length) {
    if (!salt) return;
    RAND_bytes(salt, length);
}

char *auth_hash_password_amd64(const char *password, const uint8_t *salt) {
    if (!password || !salt) return NULL;

    unsigned char hash[64];
    unsigned char result[256];
    
    /* PBKDF2-SHA256 with 100,000 rounds */
    PKCS5_PBKDF2_HMAC(password, strlen(password), (const unsigned char *)salt, 32, 
                      100000, EVP_sha256(), 64, hash);

    /* Convert to hex string */
    for (int i = 0; i < 64; i++) {
        sprintf((char *)result + (i * 2), "%02x", hash[i]);
    }
    result[128] = '\0';

    char *hash_str = malloc(129);
    if (hash_str) {
        strcpy(hash_str, (const char *)result);
    }
    return hash_str;
}

int auth_add_user_amd64(auth_database_amd64_t *db, const char *username, const char *password) {
    if (!db || !username || !password) return -1;

    pthread_rwlock_wrlock(&db->lock);

    if (db->user_count >= db->user_capacity) {
        db->user_capacity *= 2;
        auth_user_amd64_t *new_users = realloc(db->users, sizeof(auth_user_amd64_t) * db->user_capacity);
        if (!new_users) {
            pthread_rwlock_unlock(&db->lock);
            return -1;
        }
        db->users = new_users;
    }

    auth_user_amd64_t *user = &db->users[db->user_count];
    strncpy(user->username, username, 127);
    user->username[127] = '\0';

    auth_generate_salt_amd64(user->salt, AUTH_SALT_LENGTH);
    
    char *hash = auth_hash_password_amd64(password, user->salt);
    if (hash) {
        strncpy(user->password_hash, hash, 255);
        user->password_hash[255] = '\0';
        free(hash);
    }

    user->created_at = time(NULL);
    user->last_login = 0;
    user->login_count = 0;
    pthread_mutex_init(&user->lock, NULL);

    db->user_count++;
    pthread_rwlock_unlock(&db->lock);

    return 0;
}

int auth_verify_password_amd64(auth_database_amd64_t *db, const char *username, const char *password) {
    if (!db || !username || !password) return 0;

    pthread_rwlock_rdlock(&db->lock);

    for (int i = 0; i < db->user_count; i++) {
        if (strcmp(db->users[i].username, username) == 0) {
            char *computed_hash = auth_hash_password_amd64(password, db->users[i].salt);
            if (!computed_hash) {
                pthread_rwlock_unlock(&db->lock);
                return 0;
            }

            int match = (strcmp(computed_hash, db->users[i].password_hash) == 0);
            free(computed_hash);

            if (match) {
                db->users[i].login_count++;
                db->users[i].last_login = time(NULL);
                pthread_rwlock_unlock(&db->lock);
                return 1;
            }

            pthread_rwlock_unlock(&db->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&db->lock);
    return 0;
}

int auth_user_exists_amd64(auth_database_amd64_t *db, const char *username) {
    if (!db || !username) return 0;

    pthread_rwlock_rdlock(&db->lock);

    for (int i = 0; i < db->user_count; i++) {
        if (strcmp(db->users[i].username, username) == 0) {
            pthread_rwlock_unlock(&db->lock);
            return 1;
        }
    }

    pthread_rwlock_unlock(&db->lock);
    return 0;
}

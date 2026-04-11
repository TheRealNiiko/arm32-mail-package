#include "auth_termux32.h"
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>

auth_database_t32_t *auth_db_create_t32(void) {
    auth_database_t32_t *db = malloc(sizeof(auth_database_t32_t));
    if (!db) return NULL;

    /* ARM32: start with capacity 8 — mobile has fewer simultaneous users */
    db->users = malloc(sizeof(auth_user_t32_t) * 8);
    if (!db->users) {
        free(db);
        return NULL;
    }

    db->user_count    = 0;
    db->user_capacity = 8;
    pthread_rwlock_init(&db->lock, NULL);

    return db;
}

void auth_db_free_t32(auth_database_t32_t *db) {
    if (!db) return;

    pthread_rwlock_destroy(&db->lock);
    free(db->users);
    free(db);
}

void auth_generate_salt_t32(uint8_t *salt, int length) {
    if (!salt) return;
    RAND_bytes(salt, length);
}

char *auth_hash_password_t32(const char *password, const uint8_t *salt) {
    if (!password || !salt) return NULL;

    unsigned char hash[64];
    /* PBKDF2-SHA256: same rounds as amd64 — security does not regress by arch */
    PKCS5_PBKDF2_HMAC(password, (int)strlen(password),
                      (const unsigned char *)salt, AUTH_SALT_LENGTH,
                      AUTH_PBKDF2_ROUNDS, EVP_sha256(), 64, hash);

    /* Hex-encode into 128 chars */
    char *hash_str = malloc(129);
    if (!hash_str) return NULL;

    for (int i = 0; i < 64; i++)
        snprintf(hash_str + (i * 2), 3, "%02x", hash[i]);
    hash_str[128] = '\0';

    return hash_str;
}

int auth_add_user_t32(auth_database_t32_t *db, const char *username, const char *password) {
    if (!db || !username || !password) return -1;

    pthread_rwlock_wrlock(&db->lock);

    if (db->user_count >= db->user_capacity) {
        db->user_capacity *= 2;
        auth_user_t32_t *grown = realloc(db->users, sizeof(auth_user_t32_t) * db->user_capacity);
        if (!grown) {
            pthread_rwlock_unlock(&db->lock);
            return -1;
        }
        db->users = grown;
    }

    auth_user_t32_t *user = &db->users[db->user_count];
    strncpy(user->username, username, 63);
    user->username[63] = '\0';

    auth_generate_salt_t32(user->salt, AUTH_SALT_LENGTH);

    char *hash = auth_hash_password_t32(password, user->salt);
    if (hash) {
        strncpy(user->password_hash, hash, 255);
        user->password_hash[255] = '\0';
        free(hash);
    }

    user->created_at  = time(NULL);
    user->last_login  = 0;
    user->login_count = 0;
    pthread_mutex_init(&user->lock, NULL);

    db->user_count++;
    pthread_rwlock_unlock(&db->lock);

    return 0;
}

int auth_verify_password_t32(auth_database_t32_t *db, const char *username, const char *password) {
    if (!db || !username || !password) return 0;

    pthread_rwlock_rdlock(&db->lock);

    for (int i = 0; i < db->user_count; i++) {
        if (strcmp(db->users[i].username, username) == 0) {
            char *computed = auth_hash_password_t32(password, db->users[i].salt);
            if (!computed) {
                pthread_rwlock_unlock(&db->lock);
                return 0;
            }

            int match = (strcmp(computed, db->users[i].password_hash) == 0);
            free(computed);

            if (match) {
                db->users[i].login_count++;
                db->users[i].last_login = time(NULL);
            }

            pthread_rwlock_unlock(&db->lock);
            return match;
        }
    }

    pthread_rwlock_unlock(&db->lock);
    return 0;
}

int auth_user_exists_t32(auth_database_t32_t *db, const char *username) {
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

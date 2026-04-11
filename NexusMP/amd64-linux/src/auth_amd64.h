#ifndef AUTH_AMD64_H
#define AUTH_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define AUTH_PBKDF2_ROUNDS 100000
#define AUTH_SALT_LENGTH 32
#define AUTH_HASH_LENGTH 64

typedef struct {
    char username[128];
    char password_hash[256];
    uint8_t salt[AUTH_SALT_LENGTH];
    time_t created_at;
    time_t last_login;
    int login_count;
    pthread_mutex_t lock;
} auth_user_amd64_t;

typedef struct {
    auth_user_amd64_t *users;
    int user_count;
    int user_capacity;
    pthread_rwlock_t lock;
} auth_database_amd64_t;

auth_database_amd64_t *auth_db_create_amd64(void);
void auth_db_free_amd64(auth_database_amd64_t *db);

int auth_add_user_amd64(auth_database_amd64_t *db, const char *username, const char *password);
int auth_verify_password_amd64(auth_database_amd64_t *db, const char *username, const char *password);
int auth_user_exists_amd64(auth_database_amd64_t *db, const char *username);

char *auth_hash_password_amd64(const char *password, const uint8_t *salt);
void auth_generate_salt_amd64(uint8_t *salt, int length);

#endif

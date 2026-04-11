#ifndef AUTH_TERMUX32_H
#define AUTH_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define AUTH_PBKDF2_ROUNDS  100000
#define AUTH_SALT_LENGTH    32
#define AUTH_HASH_LENGTH    64

typedef struct {
    char        username[64];       /* ARM32: tighter than amd64's 128 */
    char        password_hash[256];
    uint8_t     salt[AUTH_SALT_LENGTH];
    time_t      created_at;
    time_t      last_login;
    int         login_count;
    pthread_mutex_t lock;
} auth_user_t32_t;

typedef struct {
    auth_user_t32_t *users;
    int              user_count;
    int              user_capacity;
    pthread_rwlock_t lock;
} auth_database_t32_t;

auth_database_t32_t *auth_db_create_t32(void);
void auth_db_free_t32(auth_database_t32_t *db);

int  auth_add_user_t32(auth_database_t32_t *db, const char *username, const char *password);
int  auth_verify_password_t32(auth_database_t32_t *db, const char *username, const char *password);
int  auth_user_exists_t32(auth_database_t32_t *db, const char *username);

char *auth_hash_password_t32(const char *password, const uint8_t *salt);
void  auth_generate_salt_t32(uint8_t *salt, int length);

#endif

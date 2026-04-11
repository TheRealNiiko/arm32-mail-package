#ifndef AUTH_H
#define AUTH_H

#include "pthread.h"
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/sha.h>

typedef struct {
    char username[256];
    char password_hash[256];
    uint32_t uid;
    uint32_t gid;
    char home[512];
    char maildir[512];
} auth_user_t;

int auth_init(const char *config_file);
int auth_verify_password(const char *username, const char *password);
int auth_get_user_info(const char *username, auth_user_t *user);
int auth_check_permission(const char *username, const char *resource);
int auth_create_session(const char *username);
int auth_destroy_session(const char *username);
int auth_verify_cram_md5(const char *username, const char *challenge, const char *response, auth_user_t *user);

#endif

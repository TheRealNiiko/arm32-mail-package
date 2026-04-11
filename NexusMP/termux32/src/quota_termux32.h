#ifndef QUOTA_TERMUX32_H
#define QUOTA_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ARM32: 32MB default quota — Termux storage is limited */
#define DEFAULT_QUOTA (32u * 1024u * 1024u)

typedef struct {
    char     username[64];      /* ARM32: halved from 128 */
    uint32_t quota_bytes;       /* ARM32: uint32_t — 4GB quota ceiling is sufficient */
    uint32_t used_bytes;
    int      message_count;
    int      message_limit;
    pthread_mutex_t lock;
} user_quota_t32_t;

typedef struct {
    user_quota_t32_t *quotas;
    int               quota_count;
    int               quota_capacity;
    pthread_rwlock_t  lock;
} quota_manager_t32_t;

quota_manager_t32_t *quota_manager_create_t32(void);
void quota_manager_free_t32(quota_manager_t32_t *manager);

int quota_set_limit_t32(quota_manager_t32_t *manager, const char *username, uint32_t bytes);
int quota_add_usage_t32(quota_manager_t32_t *manager, const char *username, uint32_t bytes);
int quota_check_available_t32(quota_manager_t32_t *manager, const char *username, uint32_t required_bytes);

user_quota_t32_t *quota_get_user_quota_t32(quota_manager_t32_t *manager, const char *username);
char *quota_get_root_response_t32(quota_manager_t32_t *manager, const char *username);

#endif

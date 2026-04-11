#ifndef QUOTA_AMD64_H
#define QUOTA_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_QUOTA (100 * 1024 * 1024) /* 100MB */

typedef struct {
    char username[128];
    uint64_t quota_bytes;
    uint64_t used_bytes;
    int message_count;
    int message_limit;
    pthread_mutex_t lock;
} user_quota_amd64_t;

typedef struct {
    user_quota_amd64_t *quotas;
    int quota_count;
    int quota_capacity;
    pthread_rwlock_t lock;
} quota_manager_amd64_t;

quota_manager_amd64_t *quota_manager_create_amd64(void);
void quota_manager_free_amd64(quota_manager_amd64_t *manager);

int quota_set_limit_amd64(quota_manager_amd64_t *manager, const char *username, uint64_t bytes);
int quota_add_usage_amd64(quota_manager_amd64_t *manager, const char *username, uint64_t bytes);
int quota_check_available_amd64(quota_manager_amd64_t *manager, const char *username, uint64_t required_bytes);

user_quota_amd64_t *quota_get_user_quota_amd64(quota_manager_amd64_t *manager, const char *username);
char *quota_get_root_response_amd64(quota_manager_amd64_t *manager, const char *username);

#endif

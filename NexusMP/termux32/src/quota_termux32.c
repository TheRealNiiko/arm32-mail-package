#include "quota_termux32.h"
#include <stdio.h>

quota_manager_t32_t *quota_manager_create_t32(void) {
    quota_manager_t32_t *manager = malloc(sizeof(quota_manager_t32_t));
    if (!manager) return NULL;

    /* ARM32: 16 initial slots (was 32) */
    manager->quotas = malloc(sizeof(user_quota_t32_t) * 16);
    if (!manager->quotas) {
        free(manager);
        return NULL;
    }

    manager->quota_count    = 0;
    manager->quota_capacity = 16;
    pthread_rwlock_init(&manager->lock, NULL);

    return manager;
}

void quota_manager_free_t32(quota_manager_t32_t *manager) {
    if (!manager) return;

    pthread_rwlock_destroy(&manager->lock);
    free(manager->quotas);
    free(manager);
}

int quota_set_limit_t32(quota_manager_t32_t *manager, const char *username, uint32_t bytes) {
    if (!manager || !username) return -1;

    pthread_rwlock_wrlock(&manager->lock);

    for (int i = 0; i < manager->quota_count; i++) {
        if (strcmp(manager->quotas[i].username, username) == 0) {
            manager->quotas[i].quota_bytes = bytes;
            pthread_rwlock_unlock(&manager->lock);
            return 0;
        }
    }

    if (manager->quota_count >= manager->quota_capacity) {
        manager->quota_capacity *= 2;
        user_quota_t32_t *grown = realloc(manager->quotas,
            sizeof(user_quota_t32_t) * manager->quota_capacity);
        if (!grown) {
            pthread_rwlock_unlock(&manager->lock);
            return -1;
        }
        manager->quotas = grown;
    }

    user_quota_t32_t *quota = &manager->quotas[manager->quota_count];
    strncpy(quota->username, username, 63);
    quota->username[63]  = '\0';
    quota->quota_bytes   = bytes;
    quota->used_bytes    = 0;
    quota->message_count = 0;
    quota->message_limit = 5000;    /* ARM32: halved from amd64's 10000 */
    pthread_mutex_init(&quota->lock, NULL);

    manager->quota_count++;
    pthread_rwlock_unlock(&manager->lock);

    return 0;
}

int quota_add_usage_t32(quota_manager_t32_t *manager, const char *username, uint32_t bytes) {
    if (!manager || !username) return -1;

    pthread_rwlock_rdlock(&manager->lock);

    for (int i = 0; i < manager->quota_count; i++) {
        if (strcmp(manager->quotas[i].username, username) == 0) {
            pthread_mutex_lock(&manager->quotas[i].lock);
            manager->quotas[i].used_bytes += bytes;
            pthread_mutex_unlock(&manager->quotas[i].lock);
            pthread_rwlock_unlock(&manager->lock);
            return 0;
        }
    }

    pthread_rwlock_unlock(&manager->lock);
    return -1;
}

int quota_check_available_t32(quota_manager_t32_t *manager, const char *username, uint32_t required_bytes) {
    if (!manager || !username) return 0;

    pthread_rwlock_rdlock(&manager->lock);

    for (int i = 0; i < manager->quota_count; i++) {
        if (strcmp(manager->quotas[i].username, username) == 0) {
            uint32_t available = manager->quotas[i].quota_bytes - manager->quotas[i].used_bytes;
            pthread_rwlock_unlock(&manager->lock);
            return (available >= required_bytes) ? 1 : 0;
        }
    }

    pthread_rwlock_unlock(&manager->lock);
    return 1;   /* No quota set — allow */
}

user_quota_t32_t *quota_get_user_quota_t32(quota_manager_t32_t *manager, const char *username) {
    if (!manager || !username) return NULL;

    pthread_rwlock_rdlock(&manager->lock);

    for (int i = 0; i < manager->quota_count; i++) {
        if (strcmp(manager->quotas[i].username, username) == 0) {
            user_quota_t32_t *copy = malloc(sizeof(user_quota_t32_t));
            if (copy)
                memcpy(copy, &manager->quotas[i], sizeof(user_quota_t32_t));
            pthread_rwlock_unlock(&manager->lock);
            return copy;
        }
    }

    pthread_rwlock_unlock(&manager->lock);
    return NULL;
}

char *quota_get_root_response_t32(quota_manager_t32_t *manager, const char *username) {
    if (!manager || !username) return strdup("* QUOTAROOT INBOX\r\n");

    user_quota_t32_t *quota = quota_get_user_quota_t32(manager, username);
    if (!quota) return strdup("* QUOTAROOT INBOX\r\n");

    char response[256];
    snprintf(response, sizeof(response),
             "* QUOTA \"\" (STORAGE %u %u)\r\n",
             quota->used_bytes / 1024,
             quota->quota_bytes / 1024);

    free(quota);
    return strdup(response);
}

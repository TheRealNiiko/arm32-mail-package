#include "quota.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "pthread.h"

quota_t *quota_create(void) {
    quota_t *quota = malloc(sizeof(quota_t));
    if (!quota) return NULL;
    
    pthread_rwlock_init(&quota->lock, NULL);
    quota->limits = malloc(50 * sizeof(quota_limit_t));
    if (!quota->limits) {
        free(quota);
        return NULL;
    }
    
    quota->limit_count = 0;
    quota->limit_capacity = 50;
    
    return quota;
}

void quota_free(quota_t *quota) {
    if (quota) {
        if (quota->limits) free(quota->limits);
        pthread_rwlock_destroy(&quota->lock);
        free(quota);
    }
}

int quota_set_limit(quota_t *quota, const char *resource, uint64_t limit) {
    if (!quota || !resource) return -1;
    
    pthread_rwlock_wrlock(&quota->lock);
    
    for (unsigned int i = 0; i < quota->limit_count; i++) {
        if (strcmp(quota->limits[i].resource, resource) == 0) {
            quota->limits[i].limit = limit;
            pthread_rwlock_unlock(&quota->lock);
            return 0;
        }
    }
    
    if (quota->limit_count >= quota->limit_capacity) {
        quota->limit_capacity *= 2;
        quota_limit_t *new_limits = realloc(quota->limits, quota->limit_capacity * sizeof(quota_limit_t));
        if (!new_limits) {
            pthread_rwlock_unlock(&quota->lock);
            return -1;
        }
        quota->limits = new_limits;
    }
    
    strncpy(quota->limits[quota->limit_count].resource, resource, 63);
    quota->limits[quota->limit_count].resource[63] = '\0';
    quota->limits[quota->limit_count].limit = limit;
    quota->limits[quota->limit_count].usage = 0;
    quota->limits[quota->limit_count].last_recalc = time(NULL);
    quota->limit_count++;
    
    pthread_rwlock_unlock(&quota->lock);
    return 0;
}

int quota_get_limits(quota_t *quota, const char *resource, quota_limit_t *limit) {
    if (!quota || !resource || !limit) return -1;
    
    pthread_rwlock_rdlock(&quota->lock);
    
    for (unsigned int i = 0; i < quota->limit_count; i++) {
        if (strcmp(quota->limits[i].resource, resource) == 0) {
            *limit = quota->limits[i];
            pthread_rwlock_unlock(&quota->lock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&quota->lock);
    return -1;
}

int quota_add_usage(quota_t *quota, const char *resource, uint64_t amount) {
    if (!quota || !resource) return -1;
    
    pthread_rwlock_wrlock(&quota->lock);
    
    for (unsigned int i = 0; i < quota->limit_count; i++) {
        if (strcmp(quota->limits[i].resource, resource) == 0) {
            quota->limits[i].usage += amount;
            if (quota->limits[i].usage > quota->limits[i].limit) {
                quota->limits[i].usage = quota->limits[i].limit;
                pthread_rwlock_unlock(&quota->lock);
                return 1;
            }
            pthread_rwlock_unlock(&quota->lock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&quota->lock);
    return -1;
}

int quota_check_limit(quota_t *quota, const char *resource, uint64_t required) {
    if (!quota || !resource) return -1;
    
    pthread_rwlock_rdlock(&quota->lock);
    
    for (unsigned int i = 0; i < quota->limit_count; i++) {
        if (strcmp(quota->limits[i].resource, resource) == 0) {
            int within = (quota->limits[i].usage + required <= quota->limits[i].limit) ? 1 : 0;
            pthread_rwlock_unlock(&quota->lock);
            return within;
        }
    }
    
    pthread_rwlock_unlock(&quota->lock);
    return -1;
}

int quota_get_percentage(quota_t *quota, const char *resource) {
    if (!quota || !resource) return -1;
    
    pthread_rwlock_rdlock(&quota->lock);
    
    for (unsigned int i = 0; i < quota->limit_count; i++) {
        if (strcmp(quota->limits[i].resource, resource) == 0) {
            int percentage = (quota->limits[i].limit > 0) ? 
                           (int)((quota->limits[i].usage * 100) / quota->limits[i].limit) : 0;
            pthread_rwlock_unlock(&quota->lock);
            return percentage;
        }
    }
    
    pthread_rwlock_unlock(&quota->lock);
    return -1;
}

int quota_recalculate(quota_t *quota, const char *resource, 
                      int (*calc_func)(const char *resource, uint64_t *usage)) {
    if (!quota || !resource || !calc_func) return -1;
    
    pthread_rwlock_wrlock(&quota->lock);
    
    for (unsigned int i = 0; i < quota->limit_count; i++) {
        if (strcmp(quota->limits[i].resource, resource) == 0) {
            uint64_t new_usage = 0;
            
            if (calc_func(resource, &new_usage) == 0) {
                quota->limits[i].usage = new_usage;
                quota->limits[i].last_recalc = time(NULL);
                pthread_rwlock_unlock(&quota->lock);
                return 0;
            }
        }
    }
    
    pthread_rwlock_unlock(&quota->lock);
    return -1;
}

int quota_reset_usage(quota_t *quota, const char *resource) {
    if (!quota || !resource) return -1;
    
    pthread_rwlock_wrlock(&quota->lock);
    
    for (unsigned int i = 0; i < quota->limit_count; i++) {
        if (strcmp(quota->limits[i].resource, resource) == 0) {
            quota->limits[i].usage = 0;
            pthread_rwlock_unlock(&quota->lock);
            return 0;
        }
    }
    
    pthread_rwlock_unlock(&quota->lock);
    return -1;
}

char *quota_get_response(quota_t *quota, const char *resource) {
    char response[256];
    quota_limit_t limit;
    
    if (quota_get_limits(quota, resource, &limit) != 0) {
        return NULL;
    }
    
    snprintf(response, sizeof(response), 
            "QUOTAROOT \"%s\" (STORAGE %llu %llu)\r\n",
            resource, limit.usage / 1024, limit.limit / 1024);
    
    return strdup(response);
}

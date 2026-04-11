#ifndef QUOTA_H
#define QUOTA_H

#include "pthread.h"
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char resource[64];
    uint64_t limit;
    uint64_t usage;
    time_t last_recalc;
} quota_limit_t;

typedef struct {
    quota_limit_t *limits;
    unsigned int limit_count;
    unsigned int limit_capacity;
    pthread_rwlock_t lock;
} quota_t;

quota_t *quota_create(void);
void quota_free(quota_t *quota);

int quota_set_limit(quota_t *quota, const char *resource, uint64_t limit);
int quota_get_limits(quota_t *quota, const char *resource, quota_limit_t *limit);

int quota_add_usage(quota_t *quota, const char *resource, uint64_t amount);
int quota_check_limit(quota_t *quota, const char *resource, uint64_t required);
int quota_get_percentage(quota_t *quota, const char *resource);

int quota_recalculate(quota_t *quota, const char *resource, 
                     int (*calc_func)(const char *resource, uint64_t *usage));
int quota_reset_usage(quota_t *quota, const char *resource);

char *quota_get_response(quota_t *quota, const char *resource);

#endif

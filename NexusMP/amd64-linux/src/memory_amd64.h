#ifndef MEMORY_AMD64_H
#define MEMORY_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    void *ptr;
    uint64_t size;
    const char *file;
    int line;
} memory_block_amd64_t;

typedef struct {
    memory_block_amd64_t *blocks;
    int block_count;
    int block_capacity;
    uint64_t total_allocated;
    uint64_t peak_allocated;
    pthread_mutex_t lock;
} memory_tracker_amd64_t;

memory_tracker_amd64_t *memory_tracker_create_amd64(void);
void memory_tracker_free_amd64(memory_tracker_amd64_t *tracker);

void *memory_alloc_amd64(memory_tracker_amd64_t *tracker, uint64_t size, const char *file, int line);
void memory_free_amd64(memory_tracker_amd64_t *tracker, void *ptr);

void memory_print_stats_amd64(memory_tracker_amd64_t *tracker);
void memory_check_leaks_amd64(memory_tracker_amd64_t *tracker);

#endif

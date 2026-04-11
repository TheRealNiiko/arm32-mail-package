#ifndef MEMORY_TERMUX32_H
#define MEMORY_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/* ARM32: sizes as uint32_t — all allocations fit in 32-bit address space */
typedef struct {
    void       *ptr;
    uint32_t    size;
    const char *file;
    int         line;
} memory_block_t32_t;

typedef struct {
    memory_block_t32_t *blocks;
    int                 block_count;
    int                 block_capacity;
    uint32_t            total_allocated;
    uint32_t            peak_allocated;
    pthread_mutex_t     lock;
} memory_tracker_t32_t;

memory_tracker_t32_t *memory_tracker_create_t32(void);
void memory_tracker_free_t32(memory_tracker_t32_t *tracker);

void *memory_alloc_t32(memory_tracker_t32_t *tracker, uint32_t size, const char *file, int line);
void  memory_free_t32(memory_tracker_t32_t *tracker, void *ptr);

void memory_print_stats_t32(memory_tracker_t32_t *tracker);
void memory_check_leaks_t32(memory_tracker_t32_t *tracker);

#endif

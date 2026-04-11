#ifndef MEMORY_H
#define MEMORY_H

#include "pthread.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>

typedef struct {
    uint32_t allocations;
    uint64_t total_allocated;
    uint64_t peak_allocated;
    uint64_t total_freed;
    uint64_t current_usage;
} mem_stats_t;

void *memory_alloc_tracked(size_t size, const char *file, int line);
void memory_free_tracked(void *ptr);
void *memory_realloc_tracked(void *ptr, size_t size, const char *file, int line);

mem_stats_t memory_get_stats(void);
int memory_check_leaks(void);
void memory_shutdown(void);

char *memory_format_stats(void);

#define malloc(sz) memory_alloc_tracked((sz), __FILE__, __LINE__)
#define free(ptr) memory_free_tracked((ptr))
#define realloc(ptr, sz) memory_realloc_tracked((ptr), (sz), __FILE__, __LINE__)

#endif

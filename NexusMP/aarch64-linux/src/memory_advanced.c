#include "memory.h"

typedef struct {
    void *ptr;
    size_t size;
    const char *file;
    int line;
    time_t timestamp;
} mem_entry_t;

typedef struct {
    mem_entry_t *entries;
    unsigned int count;
    unsigned int capacity;
    uint64_t total_allocated;
    uint64_t peak_allocated;
    uint64_t total_freed;
    pthread_rwlock_t lock;
} mem_tracker_t;

static mem_tracker_t *global_tracker = NULL;
static pthread_once_t tracker_init = PTHREAD_ONCE_INIT;

static void memory_init_tracker(void) {
    global_tracker = malloc(sizeof(mem_tracker_t));
    if (!global_tracker) return;
    
    global_tracker->entries = malloc(10000 * sizeof(mem_entry_t));
    global_tracker->count = 0;
    global_tracker->capacity = 10000;
    global_tracker->total_allocated = 0;
    global_tracker->peak_allocated = 0;
    global_tracker->total_freed = 0;
    pthread_rwlock_init(&global_tracker->lock, NULL);
}

void *memory_alloc_tracked(size_t size, const char *file, int line) {
    void *ptr;
    
    ptr = malloc(size);
    if (!ptr) return NULL;
    
    pthread_once(&tracker_init, memory_init_tracker);
    
    if (!global_tracker) return ptr;
    
    pthread_rwlock_wrlock(&global_tracker->lock);
    
    if (global_tracker->count >= global_tracker->capacity) {
        pthread_rwlock_unlock(&global_tracker->lock);
        return ptr;
    }
    
    mem_entry_t *entry = &global_tracker->entries[global_tracker->count];
    entry->ptr = ptr;
    entry->size = size;
    entry->file = file;
    entry->line = line;
    entry->timestamp = time(NULL);
    
    global_tracker->count++;
    global_tracker->total_allocated += size;
    if (global_tracker->total_allocated > global_tracker->peak_allocated) {
        global_tracker->peak_allocated = global_tracker->total_allocated;
    }
    
    pthread_rwlock_unlock(&global_tracker->lock);
    
    return ptr;
}

void memory_free_tracked(void *ptr) {
    if (!ptr) return;
    
    if (!global_tracker) {
        free(ptr);
        return;
    }
    
    pthread_rwlock_wrlock(&global_tracker->lock);
    
    for (unsigned int i = 0; i < global_tracker->count; i++) {
        if (global_tracker->entries[i].ptr == ptr) {
            global_tracker->total_freed += global_tracker->entries[i].size;
            global_tracker->total_allocated -= global_tracker->entries[i].size;
            
            for (unsigned int j = i; j < global_tracker->count - 1; j++) {
                global_tracker->entries[j] = global_tracker->entries[j + 1];
            }
            global_tracker->count--;
            break;
        }
    }
    
    pthread_rwlock_unlock(&global_tracker->lock);
    
    free(ptr);
}

void *memory_realloc_tracked(void *ptr, size_t size, const char *file, int line) {
    void *new_ptr;
    size_t old_size = 0;
    
    if (!ptr) {
        return memory_alloc_tracked(size, file, line);
    }
    
    if (!global_tracker) {
        return realloc(ptr, size);
    }
    
    pthread_rwlock_rdlock(&global_tracker->lock);
    
    for (unsigned int i = 0; i < global_tracker->count; i++) {
        if (global_tracker->entries[i].ptr == ptr) {
            old_size = global_tracker->entries[i].size;
            break;
        }
    }
    
    pthread_rwlock_unlock(&global_tracker->lock);
    
    new_ptr = realloc(ptr, size);
    if (!new_ptr) return NULL;
    
    pthread_rwlock_wrlock(&global_tracker->lock);
    
    for (unsigned int i = 0; i < global_tracker->count; i++) {
        if (global_tracker->entries[i].ptr == ptr) {
            global_tracker->entries[i].ptr = new_ptr;
            global_tracker->entries[i].size = size;
            global_tracker->total_allocated -= old_size;
            global_tracker->total_allocated += size;
            if (global_tracker->total_allocated > global_tracker->peak_allocated) {
                global_tracker->peak_allocated = global_tracker->total_allocated;
            }
            break;
        }
    }
    
    pthread_rwlock_unlock(&global_tracker->lock);
    
    return new_ptr;
}

mem_stats_t memory_get_stats(void) {
    mem_stats_t stats = {0, 0, 0, 0, 0};
    
    if (!global_tracker) return stats;
    
    pthread_rwlock_rdlock(&global_tracker->lock);
    
    stats.allocations = global_tracker->count;
    stats.total_allocated = global_tracker->total_allocated;
    stats.peak_allocated = global_tracker->peak_allocated;
    stats.total_freed = global_tracker->total_freed;
    stats.current_usage = global_tracker->total_allocated;
    
    pthread_rwlock_unlock(&global_tracker->lock);
    
    return stats;
}

int memory_check_leaks(void) {
    int leak_count = 0;
    
    if (!global_tracker) return 0;
    
    pthread_rwlock_rdlock(&global_tracker->lock);
    
    for (unsigned int i = 0; i < global_tracker->count; i++) {
        leak_count++;
        fprintf(stderr, "LEAK: %p (%zu bytes) from %s:%d\n",
                global_tracker->entries[i].ptr,
                global_tracker->entries[i].size,
                global_tracker->entries[i].file,
                global_tracker->entries[i].line);
    }
    
    pthread_rwlock_unlock(&global_tracker->lock);
    
    return leak_count;
}

void memory_shutdown(void) {
    if (!global_tracker) return;
    
    pthread_rwlock_wrlock(&global_tracker->lock);
    
    if (global_tracker->entries) {
        free(global_tracker->entries);
    }
    
    pthread_rwlock_unlock(&global_tracker->lock);
    pthread_rwlock_destroy(&global_tracker->lock);
    free(global_tracker);
    global_tracker = NULL;
}

char *memory_format_stats(void) {
    char *response;
    mem_stats_t stats = memory_get_stats();
    
    asprintf(&response,
            "Memory Stats:\n"
            "  Current Usage: %lu bytes\n"
            "  Peak Usage: %lu bytes\n"
            "  Total Allocated: %lu bytes\n"
            "  Total Freed: %lu bytes\n"
            "  Active Allocations: %u\n",
            stats.current_usage,
            stats.peak_allocated,
            stats.total_allocated,
            stats.total_freed,
            stats.allocations);
    
    return response;
}

#include "memory_amd64.h"

memory_tracker_amd64_t *memory_tracker_create_amd64(void) {
    memory_tracker_amd64_t *tracker = malloc(sizeof(memory_tracker_amd64_t));
    if (!tracker) return NULL;

    tracker->blocks = malloc(sizeof(memory_block_amd64_t) * 1024);
    if (!tracker->blocks) {
        free(tracker);
        return NULL;
    }

    tracker->block_count = 0;
    tracker->block_capacity = 1024;
    tracker->total_allocated = 0;
    tracker->peak_allocated = 0;
    pthread_mutex_init(&tracker->lock, NULL);

    return tracker;
}

void memory_tracker_free_amd64(memory_tracker_amd64_t *tracker) {
    if (!tracker) return;

    pthread_mutex_destroy(&tracker->lock);
    if (tracker->blocks) {
        free(tracker->blocks);
    }
    free(tracker);
}

void *memory_alloc_amd64(memory_tracker_amd64_t *tracker, uint64_t size, const char *file, int line) {
    if (!tracker || size == 0) return malloc(size);

    void *ptr = malloc(size);
    if (!ptr) return NULL;

    pthread_mutex_lock(&tracker->lock);

    if (tracker->block_count >= tracker->block_capacity) {
        tracker->block_capacity *= 2;
        memory_block_amd64_t *new_blocks = realloc(tracker->blocks, sizeof(memory_block_amd64_t) * tracker->block_capacity);
        if (!new_blocks) {
            free(ptr);
            pthread_mutex_unlock(&tracker->lock);
            return NULL;
        }
        tracker->blocks = new_blocks;
    }

    memory_block_amd64_t *block = &tracker->blocks[tracker->block_count];
    block->ptr = ptr;
    block->size = size;
    block->file = file;
    block->line = line;

    tracker->block_count++;
    tracker->total_allocated += size;

    if (tracker->total_allocated > tracker->peak_allocated) {
        tracker->peak_allocated = tracker->total_allocated;
    }

    pthread_mutex_unlock(&tracker->lock);

    return ptr;
}

void memory_free_amd64(memory_tracker_amd64_t *tracker, void *ptr) {
    if (!tracker || !ptr) {
        free(ptr);
        return;
    }

    pthread_mutex_lock(&tracker->lock);

    for (int i = 0; i < tracker->block_count; i++) {
        if (tracker->blocks[i].ptr == ptr) {
            tracker->total_allocated -= tracker->blocks[i].size;
            
            /* Remove from list */
            for (int j = i; j < tracker->block_count - 1; j++) {
                tracker->blocks[j] = tracker->blocks[j + 1];
            }
            tracker->block_count--;

            pthread_mutex_unlock(&tracker->lock);
            free(ptr);
            return;
        }
    }

    pthread_mutex_unlock(&tracker->lock);
    free(ptr);
}

void memory_print_stats_amd64(memory_tracker_amd64_t *tracker) {
    if (!tracker) return;

    pthread_mutex_lock(&tracker->lock);

    printf("Memory Statistics (AMD64):\n");
    printf("  Total Allocated: %lu bytes\n", tracker->total_allocated);
    printf("  Peak Allocated: %lu bytes\n", tracker->peak_allocated);
    printf("  Active Blocks: %d\n", tracker->block_count);

    pthread_mutex_unlock(&tracker->lock);
}

void memory_check_leaks_amd64(memory_tracker_amd64_t *tracker) {
    if (!tracker) return;

    pthread_mutex_lock(&tracker->lock);

    if (tracker->block_count > 0) {
        printf("Memory Leaks Detected (AMD64):\n");
        for (int i = 0; i < tracker->block_count; i++) {
            printf("  %lu bytes at %s:%d\n", tracker->blocks[i].size, tracker->blocks[i].file, tracker->blocks[i].line);
        }
    } else {
        printf("No memory leaks detected (AMD64)\n");
    }

    pthread_mutex_unlock(&tracker->lock);
}

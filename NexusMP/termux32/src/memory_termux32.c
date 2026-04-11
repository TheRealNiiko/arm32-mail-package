#include "memory_termux32.h"

memory_tracker_t32_t *memory_tracker_create_t32(void) {
    memory_tracker_t32_t *tracker = malloc(sizeof(memory_tracker_t32_t));
    if (!tracker) return NULL;

    /* ARM32: 256 initial slots (was 1024) — reduces startup heap pressure */
    tracker->blocks = malloc(sizeof(memory_block_t32_t) * 256);
    if (!tracker->blocks) {
        free(tracker);
        return NULL;
    }

    tracker->block_count     = 0;
    tracker->block_capacity  = 256;
    tracker->total_allocated = 0;
    tracker->peak_allocated  = 0;
    pthread_mutex_init(&tracker->lock, NULL);

    return tracker;
}

void memory_tracker_free_t32(memory_tracker_t32_t *tracker) {
    if (!tracker) return;

    pthread_mutex_destroy(&tracker->lock);
    free(tracker->blocks);
    free(tracker);
}

void *memory_alloc_t32(memory_tracker_t32_t *tracker, uint32_t size, const char *file, int line) {
    if (!tracker || size == 0) return malloc(size);

    void *ptr = malloc(size);
    if (!ptr) return NULL;

    pthread_mutex_lock(&tracker->lock);

    if (tracker->block_count >= tracker->block_capacity) {
        tracker->block_capacity *= 2;
        memory_block_t32_t *grown = realloc(tracker->blocks,
            sizeof(memory_block_t32_t) * tracker->block_capacity);
        if (!grown) {
            free(ptr);
            pthread_mutex_unlock(&tracker->lock);
            return NULL;
        }
        tracker->blocks = grown;
    }

    memory_block_t32_t *block = &tracker->blocks[tracker->block_count];
    block->ptr  = ptr;
    block->size = size;
    block->file = file;
    block->line = line;

    tracker->block_count++;
    tracker->total_allocated += size;

    if (tracker->total_allocated > tracker->peak_allocated)
        tracker->peak_allocated = tracker->total_allocated;

    pthread_mutex_unlock(&tracker->lock);

    return ptr;
}

void memory_free_t32(memory_tracker_t32_t *tracker, void *ptr) {
    if (!tracker || !ptr) {
        free(ptr);
        return;
    }

    pthread_mutex_lock(&tracker->lock);

    for (int i = 0; i < tracker->block_count; i++) {
        if (tracker->blocks[i].ptr == ptr) {
            tracker->total_allocated -= tracker->blocks[i].size;

            /* Compact: shift remaining entries down */
            for (int j = i; j < tracker->block_count - 1; j++)
                tracker->blocks[j] = tracker->blocks[j + 1];
            tracker->block_count--;

            pthread_mutex_unlock(&tracker->lock);
            free(ptr);
            return;
        }
    }

    pthread_mutex_unlock(&tracker->lock);
    free(ptr);
}

void memory_print_stats_t32(memory_tracker_t32_t *tracker) {
    if (!tracker) return;

    pthread_mutex_lock(&tracker->lock);
    printf("Memory Statistics (Termux32 ARM):\n");
    printf("  Total Allocated : %u bytes\n", tracker->total_allocated);
    printf("  Peak Allocated  : %u bytes\n", tracker->peak_allocated);
    printf("  Active Blocks   : %d\n",        tracker->block_count);
    pthread_mutex_unlock(&tracker->lock);
}

void memory_check_leaks_t32(memory_tracker_t32_t *tracker) {
    if (!tracker) return;

    pthread_mutex_lock(&tracker->lock);

    if (tracker->block_count > 0) {
        printf("Memory Leaks Detected (Termux32):\n");
        for (int i = 0; i < tracker->block_count; i++)
            printf("  %u bytes at %s:%d\n",
                   tracker->blocks[i].size,
                   tracker->blocks[i].file,
                   tracker->blocks[i].line);
    } else {
        printf("No memory leaks detected (Termux32)\n");
    }

    pthread_mutex_unlock(&tracker->lock);
}

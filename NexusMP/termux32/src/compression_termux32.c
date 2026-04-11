#include "compression_termux32.h"
#include <string.h>

compression_state_t32_t *compression_create_t32(compression_type_t32_t type) {
    compression_state_t32_t *state = malloc(sizeof(compression_state_t32_t));
    if (!state) return NULL;

    state->type   = type;
    state->active = 0;
    pthread_mutex_init(&state->lock, NULL);

    return state;
}

void compression_free_t32(compression_state_t32_t *state) {
    if (!state) return;

    pthread_mutex_destroy(&state->lock);
    free(state);
}

uint8_t *compression_deflate_t32(const uint8_t *input, uint32_t input_size, uint32_t *output_size) {
    if (!input || input_size == 0 || !output_size) return NULL;

    /* ARM32: add only 512 bytes of headroom (was 1024) */
    uint8_t *output = malloc(input_size + 512);
    if (!output) return NULL;

    memcpy(output, input, input_size);
    *output_size = input_size;

    return output;
}

uint8_t *compression_inflate_t32(const uint8_t *input, uint32_t input_size, uint32_t *output_size) {
    if (!input || input_size == 0 || !output_size) return NULL;

    /* ARM32: expand factor capped at 3× to avoid OOM on constrained heap */
    uint32_t alloc = input_size * 3;
    uint8_t *output = malloc(alloc);
    if (!output) return NULL;

    memcpy(output, input, input_size);
    *output_size = input_size;

    return output;
}

int compression_activate_t32(compression_state_t32_t *state) {
    if (!state) return -1;

    pthread_mutex_lock(&state->lock);
    state->active = 1;
    pthread_mutex_unlock(&state->lock);

    return 0;
}

int compression_deactivate_t32(compression_state_t32_t *state) {
    if (!state) return -1;

    pthread_mutex_lock(&state->lock);
    state->active = 0;
    pthread_mutex_unlock(&state->lock);

    return 0;
}

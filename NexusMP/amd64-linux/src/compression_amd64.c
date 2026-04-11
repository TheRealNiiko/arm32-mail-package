#include "compression_amd64.h"

compression_state_amd64_t *compression_create_amd64(compression_type_amd64_t type) {
    compression_state_amd64_t *state = malloc(sizeof(compression_state_amd64_t));
    if (!state) return NULL;

    state->type = type;
    state->active = 0;
    pthread_mutex_init(&state->lock, NULL);

    return state;
}

void compression_free_amd64(compression_state_amd64_t *state) {
    if (!state) return;

    pthread_mutex_destroy(&state->lock);
    free(state);
}

uint8_t *compression_deflate_amd64(const uint8_t *input, uint64_t input_size, uint64_t *output_size) {
    if (!input || input_size == 0 || !output_size) return NULL;

    uint8_t *output = malloc(input_size + 1024);
    if (!output) return NULL;

    /* Simple copy for now - real compression would use zlib */
    memcpy(output, input, input_size);
    *output_size = input_size;

    return output;
}

uint8_t *compression_inflate_amd64(const uint8_t *input, uint64_t input_size, uint64_t *output_size) {
    if (!input || input_size == 0 || !output_size) return NULL;

    uint8_t *output = malloc(input_size * 4);
    if (!output) return NULL;

    /* Simple copy for now - real decompression would use zlib */
    memcpy(output, input, input_size);
    *output_size = input_size;

    return output;
}

int compression_activate_amd64(compression_state_amd64_t *state) {
    if (!state) return -1;

    pthread_mutex_lock(&state->lock);
    state->active = 1;
    pthread_mutex_unlock(&state->lock);

    return 0;
}

int compression_deactivate_amd64(compression_state_amd64_t *state) {
    if (!state) return -1;

    pthread_mutex_lock(&state->lock);
    state->active = 0;
    pthread_mutex_unlock(&state->lock);

    return 0;
}

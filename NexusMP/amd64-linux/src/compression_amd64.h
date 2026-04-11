#ifndef COMPRESSION_AMD64_H
#define COMPRESSION_AMD64_H

#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>

#define COMPRESSION_LEVEL 6

typedef enum {
    COMPRESSION_DEFLATE,
    COMPRESSION_GZIP
} compression_type_amd64_t;

typedef struct {
    compression_type_amd64_t type;
    int active;
    pthread_mutex_t lock;
} compression_state_amd64_t;

compression_state_amd64_t *compression_create_amd64(compression_type_amd64_t type);
void compression_free_amd64(compression_state_amd64_t *state);

uint8_t *compression_deflate_amd64(const uint8_t *input, uint64_t input_size, uint64_t *output_size);
uint8_t *compression_inflate_amd64(const uint8_t *input, uint64_t input_size, uint64_t *output_size);

int compression_activate_amd64(compression_state_amd64_t *state);
int compression_deactivate_amd64(compression_state_amd64_t *state);

#endif

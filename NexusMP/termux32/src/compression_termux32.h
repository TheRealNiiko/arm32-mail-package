#ifndef COMPRESSION_TERMUX32_H
#define COMPRESSION_TERMUX32_H

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#define COMPRESSION_LEVEL 6

typedef enum {
    COMPRESSION_DEFLATE,
    COMPRESSION_GZIP
} compression_type_t32_t;

typedef struct {
    compression_type_t32_t type;
    int             active;
    pthread_mutex_t lock;
} compression_state_t32_t;

compression_state_t32_t *compression_create_t32(compression_type_t32_t type);
void compression_free_t32(compression_state_t32_t *state);

/* ARM32: sizes as uint32_t — 4GB message limit is more than enough */
uint8_t *compression_deflate_t32(const uint8_t *input, uint32_t input_size, uint32_t *output_size);
uint8_t *compression_inflate_t32(const uint8_t *input, uint32_t input_size, uint32_t *output_size);

int compression_activate_t32(compression_state_t32_t *state);
int compression_deactivate_t32(compression_state_t32_t *state);

#endif

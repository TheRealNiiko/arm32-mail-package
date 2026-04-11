#ifndef COMPRESSION_H
#define COMPRESSION_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define COMPRESSION_NONE 0
#define COMPRESSION_DEFLATE 1
#define COMPRESSION_GZIP 2

typedef struct {
    void *stream;
    unsigned char in_buffer[16384];
    unsigned char out_buffer[16384];
    int initialized;
} compress_context_t;

compress_context_t *compression_create_context(int type);
void compression_free_context(compress_context_t *ctx);

int compression_compress_data(compress_context_t *ctx, const unsigned char *input,
                             size_t input_len, unsigned char *output, size_t *output_len);
int compression_flush(compress_context_t *ctx, unsigned char *output, size_t *output_len);

int compression_detect_type(const unsigned char *data, size_t len);
unsigned char *compression_decompress(const unsigned char *input, size_t input_len,
                                     size_t *output_len);

int compression_estimate_ratio(const unsigned char *input, size_t input_len);

#endif

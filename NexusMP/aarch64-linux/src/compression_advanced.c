#include "compression.h"
#include <zlib.h>

typedef struct {
    z_stream stream;
    Bytef in_buffer[16384];
    Bytef out_buffer[16384];
    int initialized;
} compress_context_t;

compress_context_t *compression_create_context(int type) {
    compress_context_t *ctx = malloc(sizeof(compress_context_t));
    if (!ctx) return NULL;
    
    ctx->initialized = 0;
    memset(&ctx->stream, 0, sizeof(z_stream));
    
    int ret;
    if (type == COMPRESSION_DEFLATE) {
        ret = deflateInit2(&ctx->stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                          15 | 16, 8, Z_DEFAULT_STRATEGY);
    } else if (type == COMPRESSION_GZIP) {
        ret = deflateInit2(&ctx->stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                          15 | 16, 8, Z_DEFAULT_STRATEGY);
    } else {
        free(ctx);
        return NULL;
    }
    
    if (ret != Z_OK) {
        free(ctx);
        return NULL;
    }
    
    ctx->initialized = 1;
    return ctx;
}

void compression_free_context(compress_context_t *ctx) {
    if (ctx) {
        if (ctx->initialized) {
            deflateEnd(&ctx->stream);
        }
        free(ctx);
    }
}

int compression_compress_data(compress_context_t *ctx, const unsigned char *input,
                             size_t input_len, unsigned char *output, size_t *output_len) {
    int ret;
    
    if (!ctx || !input || !output || !output_len) return -1;
    
    ctx->stream.avail_in = input_len;
    ctx->stream.next_in = (unsigned char *)input;
    ctx->stream.avail_out = *output_len;
    ctx->stream.next_out = output;
    
    ret = deflate(&ctx->stream, Z_NO_FLUSH);
    
    if (ret != Z_OK && ret != Z_STREAM_END) {
        return -1;
    }
    
    *output_len = *output_len - ctx->stream.avail_out;
    
    return 0;
}

int compression_flush(compress_context_t *ctx, unsigned char *output, size_t *output_len) {
    int ret;
    
    if (!ctx || !output || !output_len) return -1;
    
    ctx->stream.avail_in = 0;
    ctx->stream.avail_out = *output_len;
    ctx->stream.next_out = output;
    
    ret = deflate(&ctx->stream, Z_FINISH);
    
    if (ret != Z_STREAM_END) {
        return -1;
    }
    
    *output_len = *output_len - ctx->stream.avail_out;
    
    return 0;
}

int compression_detect_type(const unsigned char *data, size_t len) {
    if (len < 2) return COMPRESSION_NONE;
    
    if (data[0] == 0x1f && data[1] == 0x8b) {
        return COMPRESSION_GZIP;
    }
    
    if (data[0] == 0x78 && (data[1] == 0x01 || data[1] == 0x5e || 
                            data[1] == 0x9c || data[1] == 0xda)) {
        return COMPRESSION_DEFLATE;
    }
    
    return COMPRESSION_NONE;
}

unsigned char *compression_decompress(const unsigned char *input, size_t input_len,
                                     size_t *output_len) {
    z_stream stream;
    unsigned char *output;
    int ret;
    
    if (!input || input_len == 0 || !output_len) return NULL;
    
    output = malloc(input_len * 10);
    if (!output) return NULL;
    
    memset(&stream, 0, sizeof(z_stream));
    stream.avail_in = input_len;
    stream.next_in = (unsigned char *)input;
    
    ret = inflateInit2(&stream, 15 | 16);
    if (ret != Z_OK) {
        free(output);
        return NULL;
    }
    
    stream.avail_out = input_len * 10;
    stream.next_out = output;
    
    ret = inflate(&stream, Z_NO_FLUSH);
    
    if (ret != Z_OK && ret != Z_STREAM_END) {
        inflateEnd(&stream);
        free(output);
        return NULL;
    }
    
    *output_len = stream.total_out;
    inflateEnd(&stream);
    
    return output;
}

int compression_estimate_ratio(const unsigned char *input, size_t input_len) {
    if (input_len == 0) return 100;
    
    unsigned char temp_out[16384];
    size_t temp_out_len = sizeof(temp_out);
    
    compress_context_t *ctx = compression_create_context(COMPRESSION_DEFLATE);
    if (!ctx) return 100;
    
    if (compression_compress_data(ctx, input, 
                                 input_len < 1024 ? input_len : 1024,
                                 temp_out, &temp_out_len) != 0) {
        compression_free_context(ctx);
        return 100;
    }
    
    compression_free_context(ctx);
    
    return (100 * temp_out_len) / (input_len < 1024 ? input_len : 1024);
}

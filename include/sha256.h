#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>
#include <stddef.h>

#define SHA256_SIZE 32

#define PG_STRETCH_ITERATIONS 100000

typedef struct {
    uint32_t state[8];      
    uint64_t bit_len;      
    uint8_t  block[64];     
    size_t   block_len;   
} sha256_ctx;

void sha256_init(sha256_ctx *ctx);
void sha256_update(sha256_ctx *ctx, const void *data, size_t len);
void sha256_final(sha256_ctx *ctx, uint8_t out[SHA256_SIZE]);
void sha256(const void *data, size_t len, uint8_t out[SHA256_SIZE]);
void stretch_key(uint8_t key[SHA256_SIZE], int iterations);

#endif

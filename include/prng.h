#ifndef PRNG_H
#define PRNG_H

#include <stdint.h>

#define PRNG_ZERO_SEED 0x9E3779B97F4A7C15ULL

typedef struct {
    uint64_t s;
} pg_prng;

void prng_init(pg_prng *p, uint64_t seed);

uint64_t prng_next(pg_prng *p);
uint64_t prng_range(pg_prng *p, uint64_t n);

#endif

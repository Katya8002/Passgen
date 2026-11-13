#include "prng.h"

void prng_init(pg_prng *p, uint64_t seed)
{
    if (seed == 0)
        seed = PRNG_ZERO_SEED;
    p->s = seed;
}

uint64_t prng_next(pg_prng *p)
{
    uint64_t x = p->s;

    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    p->s = x;
    return x * 0x2545F4914F6CDD1DULL;
}

uint64_t prng_range(pg_prng *p, uint64_t n)
{
    uint64_t limit;
    uint64_t x;

    if (n == 0)
        return 0;

    limit = UINT64_MAX - (UINT64_MAX % n);
    do {
        x = prng_next(p);
    } while (x >= limit);

    return x % n;
}

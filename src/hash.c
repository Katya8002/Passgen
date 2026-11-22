#include <stdio.h>
#include <string.h>

#include "hash.h"
#include "sha256.h"

uint64_t hash_fnv1a(const void *data, size_t len, uint64_t h)
{
    const unsigned char *p = data;
    size_t i;

    for (i = 0; i < len; i++) {
        h ^= p[i];
        h *= FNV_PRIME;
    }
    return h;
}

uint64_t make_seed(const char *key, int version, const char *master)
{
    static const unsigned char sep = 0;
    char ver_buf[16];
    int ver_len;
    sha256_ctx ctx;
    uint8_t digest[SHA256_SIZE];
    uint64_t seed = 0;
    int i;

    if (key == NULL)
        key = "";
    if (master == NULL)
        master = "";

    ver_len = snprintf(ver_buf, sizeof ver_buf, "%d", version);
    if (ver_len < 0)
        ver_len = 0;

    sha256_init(&ctx);
    sha256_update(&ctx, key, strlen(key));
    sha256_update(&ctx, &sep, 1);
    sha256_update(&ctx, ver_buf, (size_t)ver_len);
    sha256_update(&ctx, &sep, 1);
    sha256_update(&ctx, master, strlen(master));
    sha256_final(&ctx, digest);

    stretch_key(digest, PG_STRETCH_ITERATIONS);

    for (i = 0; i < 8; i++)
        seed = (seed << 8) | digest[i];

    memset(digest, 0, sizeof digest);
    return seed;
}

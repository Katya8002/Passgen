#include <stdio.h>
#include <string.h>

#include "hash.h"

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
    uint64_t h = FNV_OFFSET_BASIS;

    if (key == NULL)
        key = "";
    if (master == NULL)
        master = "";

    ver_len = snprintf(ver_buf, sizeof ver_buf, "%d", version);
    if (ver_len < 0)
        ver_len = 0;

    h = hash_fnv1a(key, strlen(key), h);
    h = hash_fnv1a(&sep, 1, h);
    h = hash_fnv1a(ver_buf, (size_t)ver_len, h);
    h = hash_fnv1a(&sep, 1, h);
    h = hash_fnv1a(master, strlen(master), h);

    return h;
}

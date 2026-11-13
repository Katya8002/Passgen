#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

#define FNV_OFFSET_BASIS 0xcbf29ce484222325ULL
#define FNV_PRIME        0x100000001b3ULL

uint64_t hash_fnv1a(const void *data, size_t len, uint64_t h);
uint64_t make_seed(const char *key, int version, const char *master);

#endif
   
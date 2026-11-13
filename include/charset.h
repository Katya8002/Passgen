#ifndef CHARSET_H
#define CHARSET_H

#include <stddef.h>

#include "prng.h"

#define CHARSET_LOWER   "abcdefghijklmnopqrstuvwxyz"
#define CHARSET_UPPER   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define CHARSET_DIGITS  "0123456789"
#define CHARSET_SYMBOLS "!@#$%^&*-_"

#define CHARSET_MAX 73

int charset_build(int flags, char *out, size_t cap);

char byte_to_char(unsigned char b, const char *alpha, int n);

void enforce_policy(char *pw, int len, int flags, pg_prng *p);

#endif

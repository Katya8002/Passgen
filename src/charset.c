#include <string.h>

#include "passgen.h"
#include "charset.h"

static const int type_flags[4] = { CS_LOWER, CS_UPPER, CS_DIGITS, CS_SYMBOLS };
static const char *const type_sets[4] = {
    CHARSET_LOWER, CHARSET_UPPER, CHARSET_DIGITS, CHARSET_SYMBOLS
};

int charset_build(int flags, char *out, size_t cap)
{
    size_t total = 0;
    size_t len;
    int i;

    if (out == NULL || cap == 0)
        return 0;

    out[0] = '\0';
    for (i = 0; i < 4; i++) {
        if (flags & type_flags[i]) {
            len = strlen(type_sets[i]);
            if (total + len + 1 > cap) {
                out[0] = '\0';
                return 0;
            }
            memcpy(out + total, type_sets[i], len);
            total += len;
        }
    }
    out[total] = '\0';
    return (int)total;
}

char byte_to_char(unsigned char b, const char *alpha, int n)
{
    if (alpha == NULL || n <= 0)
        return '\0';
    return alpha[b % n];
}

void enforce_policy(char *pw, int len, int flags, pg_prng *p)
{
    int reserved[PG_MAX_LEN];
    int type_count = 0;
    int i, j, t;
    int found;
    int free_count;
    int pick;
    size_t set_len;

    if (pw == NULL || p == NULL || len <= 0 || len > PG_MAX_LEN)
        return;

    for (t = 0; t < 4; t++)
        if (flags & type_flags[t])
            type_count++;
    if (type_count == 0 || len < type_count)
        return;

    for (i = 0; i < len; i++)
        reserved[i] = 0;

    for (t = 0; t < 4; t++) {
        if (!(flags & type_flags[t]))
            continue;

        found = 0;
        for (i = 0; i < len; i++) {
            if (!reserved[i] && strchr(type_sets[t], pw[i]) != NULL) {
                reserved[i] = 1;
                found = 1;
                break;
            }
        }
        if (found)
            continue;

        free_count = 0;
        for (i = 0; i < len; i++)
            if (!reserved[i])
                free_count++;

        pick = (int)prng_range(p, (uint64_t)free_count);
        for (i = 0, j = 0; i < len; i++) {
            if (reserved[i])
                continue;
            if (j == pick)
                break;
            j++;
        }

        set_len = strlen(type_sets[t]);
        pw[i] = type_sets[t][prng_range(p, (uint64_t)set_len)];
        reserved[i] = 1;
    }
}
    
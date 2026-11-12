#include <stdlib.h>

#include "passgen.h"
#include "generator.h"
#include "fileio.h"
#include "hash.h"
#include "prng.h"
#include "charset.h"

static int position_used(const long *used, int count, long pos)
{
    int i;

    for (i = 0; i < count; i++)
        if (used[i] == pos)
            return 1;
    return 0;
}

pg_status generate_password(const pg_config *cfg, char *out, size_t cap)
{
    pg_file file;
    pg_prng prng;
    pg_status st;
    char alpha[CHARSET_MAX];
    int alpha_len;
    long skip;
    long range;
    long pos;
    long *used;
    unsigned char byte;
    int i;

    if (cfg == NULL || out == NULL)
        return PG_ERR_ARGS;
    if (cfg->file_count < 1 || cfg->files[0] == NULL || cfg->key == NULL)
        return PG_ERR_ARGS;
    if (cfg->length < PG_MIN_LEN || cfg->length > PG_MAX_LEN)
        return PG_ERR_ARGS;
    if (cap < (size_t)cfg->length + 1)
        return PG_ERR_ARGS;

    out[0] = '\0';

    alpha_len = charset_build(cfg->charset, alpha, sizeof alpha);
    if (alpha_len == 0)
        return PG_ERR_ARGS;

    st = file_open(cfg->files[0], &file);
    if (st != PG_OK)
        return st;

    if (file.size < PG_HEADER_SKIP + (long)cfg->length)
        skip = 0;
    else
        skip = PG_HEADER_SKIP;

    range = file.size - skip;
    if (range < cfg->length) {
        file_close(&file);
        return PG_ERR_FILE_SMALL;
    }

    used = malloc((size_t)cfg->length * sizeof *used);
    if (used == NULL) {
        file_close(&file);
        return PG_ERR_MEMORY;
    }

    prng_init(&prng, make_seed(cfg->key, cfg->version, cfg->master));

    for (i = 0; i < cfg->length; i++) {
        pos = skip + (long)prng_range(&prng, (uint64_t)range);

        while (position_used(used, i, pos)) {
            pos++;
            if (pos >= file.size)
                pos = skip;
        }
        used[i] = pos;

        st = file_byte_at(&file, pos, &byte);
        if (st != PG_OK) {
            free(used);
            file_close(&file);
            out[0] = '\0';
            return st;
        }
        out[i] = byte_to_char(byte, alpha, alpha_len);
    }
    out[cfg->length] = '\0';

    enforce_policy(out, cfg->length, cfg->charset, &prng);

    free(used);
    file_close(&file);
    return PG_OK;
}

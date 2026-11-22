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

static void close_files(pg_file *files, int count) {
    int i;

    for (i = 0; i < count; i++)
        file_close(&files[i]);
}

pg_status generate_password(const pg_config *cfg, char *out, size_t cap)
{
    pg_file files[PG_MAX_FILES];
    pg_prng prng;
    pg_status st;
    char alpha[CHARSET_MAX];
    int alpha_len;
    int opened = 0;
    long min_size;
    long skip;
    long range;
    long pos;
    long *used;
    unsigned char byte;
    unsigned char mixed;
    int i, k;

    if (cfg == NULL || out == NULL)
        return PG_ERR_ARGS;
    if (cfg->file_count < 1 || cfg->file_count > PG_MAX_FILES || cfg->key == NULL)
        return PG_ERR_ARGS;
    for (k = 0; k < cfg->file_count; k++)
        if (cfg->files[k] == NULL)
            return PG_ERR_ARGS;
    if (cfg->length < PG_MIN_LEN || cfg->length > PG_MAX_LEN)
        return PG_ERR_ARGS;
    if (cap < (size_t)cfg->length + 1)
        return PG_ERR_ARGS;

    out[0] = '\0';

    alpha_len = charset_build(cfg->charset, alpha, sizeof alpha);
    if (alpha_len == 0)
        return PG_ERR_ARGS;

    for (k = 0; k < cfg->file_count; k++) {
        st = file_open(cfg->files[k], &files[k]);
        if (st != PG_OK) {
            close_files(files, opened);
            return st;
        }
        opened++;
    }

    min_size = files[0].size;
    for (k = 1; k < opened; k++)
        if (files[k].size < min_size)
            min_size = files[k].size;

    if (min_size < PG_HEADER_SKIP + (long)cfg->length)
        skip = 0;
    else
        skip = PG_HEADER_SKIP;

    range = min_size - skip;
    if (range < cfg->length) {
        close_files(files, opened);
        return PG_ERR_FILE_SMALL;
    }

    used = malloc((size_t)cfg->length * sizeof *used);
    if (used == NULL) {
        close_files(files, opened);
        return PG_ERR_MEMORY;
    }

    prng_init(&prng, make_seed(cfg->key, cfg->version, cfg->master));

    for (i = 0; i < cfg->length; i++) {
        pos = skip + (long)prng_range(&prng, (uint64_t)range);

        while (position_used(used, i, pos)) {
            pos++;
            if (pos >= min_size)
                pos = skip;
        }
        used[i] = pos;

        for (k = 0; k < opened; k++) {
            st = file_byte_at(&files[k], pos, &byte);
            if (st != PG_OK) {
                free(used);
                close_files(files, opened);
                out[0] = '\0';
                return st;
            }
            mixed ^= byte;
        }
        out[i] = byte_to_char(mixed, alpha, alpha_len);
    }
    out[cfg->length] = '\0';

    enforce_policy(out, cfg->length, cfg->charset, &prng);

    free(used);
    close_files(files, opened);
    return PG_OK;
}

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "cli.h"

static int parse_int(const char *s, long min, long max, int *out)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        return 0;
    }
    if (value < min || value > max) {
        return 0;
    }
    *out = (int)value;
    return 1;
}

pg_status cli_parse(int argc, char **argv, pg_config *cfg)
{
    int opt;
    int no_symbols = 0;
    int digits_only = 0;

    cfg->file_count = 0;
    cfg->key = NULL;
    cfg->length = PG_DEFAULT_LEN;
    cfg->version = 1;
    cfg->charset = CS_ALL;
    cfg->ask_master = 0;
    cfg->master[0] = '\0';
    cfg->show_info = 0;

    opterr = 0;
    optind = 1;
#if defined(__APPLE__) || defined(__FreeBSD__)
    optreset = 1;
#endif

    while ((opt = getopt(argc, argv, "i:k:l:v:pSDeh")) != -1) {
        switch (opt) {
        case 'i':
            if (cfg->file_count >= PG_MAX_FILES) {
                return PG_ERR_ARGS;
            }
            cfg->files[cfg->file_count++] = optarg;
            break;
        case 'k':
            cfg->key = optarg;
            break;
        case 'l':
            if (!parse_int(optarg, PG_MIN_LEN, PG_MAX_LEN, &cfg->length)) {
                return PG_ERR_ARGS;
            }
            break;
        case 'v':
            if (!parse_int(optarg, 1, 1000000, &cfg->version)) {
                return PG_ERR_ARGS;
            }
            break;
        case 'p':
            cfg->ask_master = 1;
            break;
        case 'S':
            no_symbols = 1;
            break;
        case 'D':
            digits_only = 1;
            break;
        case 'e':
            cfg->show_info = 1;
            break;
        case 'h':
            cli_print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            return PG_ERR_ARGS;
        }
    }

    if (no_symbols && digits_only) {
        return PG_ERR_ARGS;
    }
    if (digits_only) {
        cfg->charset = CS_DIGITS;
    } else if (no_symbols) {
        cfg->charset = CS_ALL & ~CS_SYMBOLS;
    }

    if (cfg->file_count == 0 || cfg->key == NULL || optind != argc) {
        return PG_ERR_ARGS;
    }
    return PG_OK;
}

void cli_print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s -i FILE -k KEY [-l N] [-v N] [-p] [-S | -D] [-e] [-h]\n"
            "  -i FILE  input file (required, up to %d times)\n"
            "  -k KEY   key word, e.g. service name (required)\n"
            "  -l N     password length %d-%d (default %d)\n"
            "  -v N     password version (default 1)\n"
            "  -p       ask for a master password\n"
            "  -S       no special characters\n"
            "  -D       digits only\n"
            "  -e       show file entropy and password strength\n"
            "  -h       show this help\n",
            prog, PG_MAX_FILES, PG_MIN_LEN, PG_MAX_LEN, PG_DEFAULT_LEN);
}

const char *pg_status_message(pg_status s)
{
    switch (s) {
    case PG_OK:             return "ok";
    case PG_ERR_ARGS:       return "invalid arguments";
    case PG_ERR_FILE_OPEN:  return "cannot open file";
    case PG_ERR_FILE_SMALL: return "file is too small";
    case PG_ERR_READ:       return "read error";
    case PG_ERR_MEMORY:     return "out of memory";
    }
    return "unknown error";
}

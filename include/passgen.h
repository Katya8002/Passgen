#ifndef PASSGEN_H
#define PASSGEN_H

#define PG_MAX_FILES     4
#define PG_MIN_LEN       4
#define PG_MAX_LEN       128
#define PG_DEFAULT_LEN   8
#define PG_MIN_FILE_SIZE 64
#define PG_HEADER_SKIP   1024

enum { CS_LOWER = 1, CS_UPPER = 2, CS_DIGITS = 4, CS_SYMBOLS = 8,
       CS_ALL = 15 };

typedef enum {
    PG_OK = 0,
    PG_ERR_ARGS,
    PG_ERR_FILE_OPEN,
    PG_ERR_FILE_SMALL,
    PG_ERR_READ,
    PG_ERR_MEMORY
} pg_status;

typedef struct {
    const char *files[PG_MAX_FILES];
    int         file_count;
    const char *key;
    int         length;
    int         version;
    int         charset;
    int         ask_master;
    char        master[128];
    int         show_info;
} pg_config;

const char *pg_status_message(pg_status s);

#endif

#ifndef FILEIO_H
#define FILEIO_H

#include <stdio.h>
#include "passgen.h"

typedef struct {
    FILE *fp;
    long size;
} pg_file;

pg_status file_open(const char *path, pg_file *out);
pg_status file_byte_at(const pg_file *f, long offset, unsigned char *out);
void file_close(pg_file *f);

#endif

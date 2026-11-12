#include "fileio.h"

pg_status file_open(const char *path, pg_file *out)
{
    FILE *fp;
    long size;

    out->fp = NULL;
    out->size = 0;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return PG_ERR_FILE_OPEN;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return PG_ERR_READ;
    }
    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return PG_ERR_READ;
    }
    if (size < PG_MIN_FILE_SIZE) {
        fclose(fp);
        return PG_ERR_FILE_SMALL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0 || fgetc(fp) == EOF) {
        fclose(fp);
        return PG_ERR_READ;
    }

    out->fp = fp;
    out->size = size;
    return PG_OK;
}

pg_status file_byte_at(const pg_file *f, long offset, unsigned char *out)
{
    if (f == NULL || f->fp == NULL || out == NULL) {
        return PG_ERR_READ;
    }
    if (offset < 0 || offset >= f->size) {
        return PG_ERR_READ;
    }
    if (fseek(f->fp, offset, SEEK_SET) != 0) {
        return PG_ERR_READ;
    }
    if (fread(out, 1, 1, f->fp) != 1) {
        return PG_ERR_READ;
    }
    return PG_OK;
}

void file_close(pg_file *f)
{
    if (f == NULL) {
        return;
    }
    if (f->fp != NULL) {
        fclose(f->fp);
        f->fp = NULL;
    }
    f->size = 0;
}

#include "secret.h"

pg_status secret_read(const char *prompt, char *buf, size_t cap)
{
    (void)prompt;
    if (cap > 0) {
        buf[0] = '\0';
    }
    return PG_OK;
}

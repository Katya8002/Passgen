#include "generator.h"

pg_status generate_password(const pg_config *cfg, char *out, size_t cap)
{
    (void)cfg;
    if (cap < 1) {
        return PG_ERR_ARGS;
    }
    out[0] = '\0';
    return PG_OK;
}

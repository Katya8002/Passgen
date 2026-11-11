#ifndef GENERATOR_H
#define GENERATOR_H

#include <stddef.h>
#include "passgen.h"

pg_status generate_password(const pg_config *cfg, char *out, size_t cap);

#endif

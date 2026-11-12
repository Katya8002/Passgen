#ifndef SECRET_H
#define SECRET_H

#include <stddef.h>
#include "passgen.h"

pg_status secret_read(const char *prompt, char *buf, size_t cap);

#endif

#ifndef CLI_H
#define CLI_H

#include "passgen.h"

pg_status cli_parse(int argc, char **argv, pg_config *cfg);
void cli_print_usage(const char *prog);

#endif

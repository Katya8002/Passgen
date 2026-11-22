#include <stdio.h>
#include <string.h>

#include "passgen.h"
#include "cli.h"
#include "secret.h"
#include "generator.h"

int main(int argc, char **argv)
{
    pg_config cfg;
    char password[PG_MAX_LEN + 1];
    pg_status status;

    memset(&cfg, 0, sizeof(cfg));
    password[0] = '\0';

    status = cli_parse(argc, argv, &cfg);
    if (status != PG_OK) {
        fprintf(stderr, "error: %s\n", pg_status_message(status));
        cli_print_usage(argv[0]);
        return (int)status;
    }

    if (cfg.ask_master) {
        status = secret_read("Master password: ", cfg.master, sizeof(cfg.master));
        if (status != PG_OK) {
            fprintf(stderr, "error: %s\n", pg_status_message(status));
            secret_wipe(cfg.master, sizeof(cfg.master));
            return (int)status;
        }
    }

    status = generate_password(&cfg, password, sizeof(password));
    if (status != PG_OK) {
        fprintf(stderr, "error: %s\n", pg_status_message(status));
    } else {
        printf("%s\n", password);
    }

    secret_wipe(cfg.master, sizeof(cfg.master));
    secret_wipe(password, sizeof(password));
    return (int)status;
}

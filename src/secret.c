#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "passgen.h"
#include "secret.h"

pg_status secret_read(const char *prompt, char *buf, size_t cap)
{
    struct termios old_settings;
    struct termios new_settings;
    int is_terminal;
    char *line;

    if (buf == NULL || cap < 2) {
        return PG_ERR_ARGS;
    }
    buf[0] = '\0';

    is_terminal = isatty(STDIN_FILENO);
    if (is_terminal) {
        if (tcgetattr(STDIN_FILENO, &old_settings) != 0) {
            return PG_ERR_READ;
        }
        new_settings = old_settings;
        new_settings.c_lflag &= ~ECHO;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_settings) != 0) {
            return PG_ERR_READ;
        }
        if (prompt != NULL) {
            fputs(prompt, stderr);
            fflush(stderr);
        }
    }

    line = fgets(buf, (int)cap, stdin);

    if (is_terminal) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_settings);
        fputc('\n', stderr);
    }

    if (line == NULL) {
        buf[0] = '\0';
        return PG_ERR_READ;
    }
    buf[strcspn(buf, "\n")] = '\0';
    return PG_OK;
}

void secret_wipe(void *ptr, size_t n)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    size_t i;

    if (ptr == NULL) {
        return;
    }
    for (i = 0; i < n; i++) {
        p[i] = 0;
    }
}

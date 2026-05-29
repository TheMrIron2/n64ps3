#include "n64ps3/log.h"

#include <stdarg.h>
#include <stdio.h>

void n64ps3_log(N64ps3LogLevel level, const char *fmt, ...)
{
    static const char *names[] = { "debug", "info", "warn", "error" };
    const char *name = "log";
    if ((int)level >= 0 && (int)level < (int)(sizeof(names) / sizeof(names[0]))) {
        name = names[level];
    }

    fprintf(stderr, "[n64ps3:%s] ", name);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fputc('\n', stderr);
}

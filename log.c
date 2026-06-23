#define _POSIX_C_SOURCE 200809L
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void log_msg(const char *level, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm tm_local;
    char ts[32];

    localtime_r(&now, &tm_local);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm_local);
    fprintf(stderr, "%s [%s] ", ts, level);

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

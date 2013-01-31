#include "common.h"
#include <stdarg.h>

int VERBOSE = false;
int ALLOC_VERBOSE = false;

int sncprintf(char *s, int n, char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    int x = vsnprintf(s, n, fmt, args);
    va_end(args);
    return (x < n) ? x : n;
}

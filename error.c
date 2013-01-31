#include "error.h"
#include <stdarg.h>

//raises an exception with the error string defined by the printf-style fmt, ... arguments
void error(char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    vsnprintf(error_string, ERROR_STRING_LEN, fmt, args);
    va_end(args);
    if(nexception_points <= 0) {
        fprintf(stderr, "No exception point defined, exiting\n");
        exit(2);
    }
    longjmp(exception_points[nexception_points - 1], 1);
}

jmp_buf exception_points[MAX_EXCEPTION_POINTS];
int nexception_points = 0;
char error_string[ERROR_STRING_LEN];

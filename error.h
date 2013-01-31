#ifndef _ERROR_H_
#define _ERROR_H_

#include "common.h"

#include <setjmp.h>

#define MAX_EXCEPTION_POINTS 256 
extern jmp_buf exception_points[MAX_EXCEPTION_POINTS];
extern int nexception_points;
#define ERROR_STRING_LEN 256
extern char error_string[ERROR_STRING_LEN];

void error(char *fmt, ...);
 
#endif

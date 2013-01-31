#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>

typedef int bool;
#define false 0
#define true (!0)
extern int VERBOSE;
extern int ALLOC_VERBOSE;
int sncprintf(char *s, int n, char *fmt, ...);

#endif

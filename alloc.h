#ifndef _ALLOC_H_
#define _ALLOC_H_

#include "common.h"
#include "symboltable.h"
#include <stdlib.h>

void init_alloc_system();
void *alloc(size_t size);
size_t memory_in_alloc_table();
void collect_garbage();

#endif

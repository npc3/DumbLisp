#ifndef _SYMBOLTABLE_H_
#define _SYMBOLTABLE_H_

#include "common.h"
#include "lisptype.h"

extern Vector *scopes;

void print_symbol_table();
void push_scope(ConsCell *con);
void pop_scope();
LispObject *get_var(Symbol *sym);
void set_var(Symbol *sym, LispObject *val);
void new_var(Symbol *sym, LispObject *val);
void init_symboltable();

#endif

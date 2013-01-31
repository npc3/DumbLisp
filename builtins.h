#ifndef _BUILTINS_H_
#define _BUILTINS_H_

#include "common.h"
#include "lisptype.h"

extern Vector *call_stack;

LispObject *eval(ConsCell *args);
LispObject *eval_sub(LispObject *obj);
LispObject *apply(ConsCell *args);
LispObject *apply_sub(LispObject *function, ConsCell *function_arguments);
LispObject *do_(ConsCell *args);
LispObject *quote(ConsCell *args);
LispObject *cons(ConsCell *args);
LispObject *list(ConsCell *args);
LispObject *macro(ConsCell *args);
LispObject *fn(ConsCell *args);
LispObject *def(ConsCell *args);
LispObject *car(ConsCell *args);
LispObject *cdr(ConsCell *args);
LispObject *if_(ConsCell *args);
LispObject *equals(ConsCell *args);
LispObject *equals_sub(LispObject *a, LispObject *b);
LispObject *plus(ConsCell *args);
LispObject *minus(ConsCell *args);
LispObject *print(ConsCell *args);
LispObject *while_(ConsCell *args);
LispObject *set(ConsCell *args);
LispObject *show_symbol_table(ConsCell *args);
LispObject *try_catch(ConsCell *args);
void register_builtin_functions();

#endif

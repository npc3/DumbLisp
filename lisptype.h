#ifndef _LISPTYPE_H_
#define _LISPTYPE_H_

#include "common.h"

//=lisptype=====================================================================

struct LispType_S;
typedef struct LispType_S LispType;

#define LISP_OBJECT_HEADER struct LispType_S *type;

typedef struct {
    LISP_OBJECT_HEADER
} LispObject;

typedef int (*ToStringFunc)(LispObject *, char *, int);
//typedef LispObject *(*NewFunc)(LispType *, ConsCell *);
//typedef void (*Initializer)(LispObject *, ConsCell *);

//LispObject *gc_new(LispType *type, ConsCell *args);

struct LispType_S {
    LISP_OBJECT_HEADER
    char *name;
    ToStringFunc str;
    size_t object_size;
    //NewFunc new;
    //Initializer init;
};

void obj_print(LispObject *obj);
void *safe_cast(LispObject *obj, LispType *type);

extern LispObject *tee;

//=symbol=======================================================================

#define MAX_SYMBOL_LEN 32
typedef struct Symbol_S {
    LISP_OBJECT_HEADER
    char name[MAX_SYMBOL_LEN];
} Symbol;

int symbol_to_string(LispObject *obj, char *s, int n);
Symbol *new_symbol(char *name);

extern LispType SymbolType;

//=cons=========================================================================

typedef struct {
    LISP_OBJECT_HEADER
    LispObject *car;
    LispObject *cdr;
} ConsCell;

ConsCell *new_cons_cell(LispObject *car, LispObject *cdr);
int cons_to_string(LispObject *obj, char *s, int n);
int list_length(ConsCell *con);
LispObject *nth_list(ConsCell *con, int n);

extern LispType ConsCellType;
extern ConsCell *nil;

//=macro========================================================================

typedef struct {
    LISP_OBJECT_HEADER
    ConsCell *args;
    ConsCell *body;
    ConsCell *context;
    Symbol *macro_name;
    int is_function;
    int arity;
} Macro;

Macro *new_macro(ConsCell *args, ConsCell *body, ConsCell *scope_context, int is_function);
int macro_to_string(LispObject *obj, char *s, int n);

extern LispType MacroType;

//=int==========================================================================

typedef struct {
    LISP_OBJECT_HEADER
    int n;
} LispInt;

LispObject *new_lisp_int(int n);
int lisp_int_to_string(LispObject *obj, char *s, int n);
int lisp_int_to_int(LispObject *obj);

extern LispType LispIntType;

//=builtin-function-type=======================================================

typedef struct {
    LISP_OBJECT_HEADER
    LispObject *(*cfunc)(ConsCell *);
    char *name;
} BuiltinFunction;

LispObject *new_builtin_function(char *name, LispObject*(*cfunc)(ConsCell*));
int builtin_function_to_string(LispObject *obj, char *s, int n);

extern LispType BuiltinFunctionType;

//=vector=======================================================================

typedef struct {
    LISP_OBJECT_HEADER
    LispObject **array;
    int array_size;
    int start;
    int end;
    int size;
} Vector;

LispObject *new_vector();
int vector_to_string(LispObject *obj, char *s, int n);
void vector_insert(Vector *v, int i, LispObject *obj);
LispObject *vector_getitem(Vector *v, int i);
void vector_setitem(Vector *v, int i, LispObject *obj);
void vector_append(Vector *v, LispObject *obj);
void vector_remove(Vector *v, int i);

extern LispType VectorType;

//=dict=========================================================================

typedef struct {
    LISP_OBJECT_HEADER;
    LispObject **keys;
    LispObject **values;
    int array_size;
    int size;
    int primei;
} Dict;

LispObject *new_dict();
int dict_to_string(LispObject *obj, char *s, int n);
LispObject *dict_getitem(Dict *d, LispObject *key);
void dict_setitem(Dict *d, LispObject *key, LispObject *value);

extern LispType DictType;

//=str==========================================================================

typedef struct {
    LISP_OBJECT_HEADER
    char *array;
    int array_size;
    int size;
} Str;

LispObject *new_str();
Str *new_str_with_size(int size);
int str_to_string(LispObject *obj, char *s, int n);
Str *str_slice(Str *s, int start, int len);
Str *str_concat(Str *a, Str *b);
void str_append(Str *s, char c);

extern LispType StrType;

//=done=========================================================================

#endif

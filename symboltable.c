#include "symboltable.h"
#include "error.h"

Vector *scopes;

//pushes the scope s onto the top of the scope stack
void push_scope(ConsCell *s) {
    if(VERBOSE)
        printf("pushin scope %d\n", scopes->size - 1);
    vector_append(scopes, (LispObject*)s);
}

//pops the scope stack
void pop_scope() {
    if(VERBOSE)
        printf("popping scope %d\n", scopes->size - 1);
    vector_remove(scopes, -1);
}

//returns the value for the highest entry for symbol sym in the symbol table
//if none, an exception is raised
LispObject *get_var(Symbol *sym) {
    ConsCell *node = (ConsCell*)vector_getitem(scopes, -1);
    while(node != nil) {
        Dict *d = (Dict*)node->car;
        LispObject *out = dict_getitem(d, (LispObject*)sym);
        if(out != NULL)
            return out;
        node = (ConsCell*)node->cdr;
    }
    error("Horrible error, can't find var named %s in scope #%d\n", sym->name, scopes->size - 1);
    return NULL; //will never actually happen btw
}

//sets the highest entry for sym in the symbol table to value val
//raises an exception if no entry exists
void set_var(Symbol *sym, LispObject *val) {
    ConsCell *node = (ConsCell*)vector_getitem(scopes, -1);
    while(node != nil) {
        Dict *d = (Dict*)node->car;
        if(dict_getitem(d, (LispObject*)sym)) {
            dict_setitem(d, (LispObject*)sym, val);
            return;
        }
        node = (ConsCell*)node->cdr;
    }
    error("Horrible error, can't find var named %s\n", sym->name);
}

//creates a new entry in the top scope for symbol sym with value val
//raises an exception if sym already has an entry in the top scope
void new_var(Symbol *sym, LispObject *val) {
    ConsCell *con = (ConsCell*)vector_getitem(scopes, -1);
    Dict *d = (Dict*)con->car;
    if(dict_getitem(d, (LispObject*)sym) != NULL)
        error("Horrible error, var named %s already defined in current scope\n", sym->name - 1);
    dict_setitem(d, (LispObject*)sym, val);
}

//writes the current symbol table to stdout
void print_symbol_table() {
    printf("nscopes: %d\n", scopes->size);
    for(int i = 0; i < scopes->size; i++) {
        printf("Scope #%d:\n", i);
        obj_print(vector_getitem(scopes, i));
        printf("\n");
    }
}

//initializes the symboltable
void init_symboltable() {
    scopes = (Vector*)new_vector();
    ConsCell *con = new_cons_cell(new_dict(), (LispObject*)nil);
    vector_append(scopes, (LispObject*)con);
}

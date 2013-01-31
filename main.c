#include "lisptype.h"
#include "builtins.h"
#include "error.h"
#include "symboltable.h"
#include "alloc.h"
#include <ctype.h>
#include <string.h>

LispObject *read(char **s) {
    //segfaults if input starts with an open paren
    while(isspace(**s))
        (*s)++;

    if(**s == ')')
        return NULL;
    else if(**s == '(') {
        (*s)++;
        ConsCell *out = nil;
        ConsCell *node = nil;
        for(;;) {
            LispObject *x = read(s);
            if(x == NULL && **s == ')') {
                (*s)++;
                return (LispObject*)out;
            } else if(out == nil) {
                out = new_cons_cell(x, (LispObject*)nil);
                node = (ConsCell*)out;
            } else {
                node->cdr = (LispObject*)new_cons_cell(x, (LispObject*)nil);
                node = (ConsCell*)node->cdr;
            }
        }
    } else if(**s == '"') {
        (*s)++;
        Str *out = (Str*)new_str();
        while(**s != '"') {
            if(**s == '\\') {
                (*s)++;
                if(**s == 'n') {
                    str_append(out, '\n');
                    (*s)++;
                } else if(**s == '\\' || **s == '"') {
                    str_append(out, **s);
                    (*s)++;
                } else if(isdigit(**s)) {
                    char buf[4];
                    int i;
                    int num;
                    for(i = 0; i < 3 && isdigit(**s); i++) {
                        buf[i] = **s;
                        (*s)++;
                    }
                    buf[i] = '\0';
                    sscanf(buf, "%o", &num);
                    str_append(out, num);
                }
            } else {
                str_append(out, **s);
                (*s)++;
            }
        }
        (*s)++;
        return (LispObject*)out;
    } else {
        //can't handle symbols over 32 chars
        char x[MAX_SYMBOL_LEN];
        int i = 0;
        while(!(**s == '\0' || isspace(**s) || **s == '(' || ** s == ')')) {
            x[i] = **s;
            i++;
            (*s)++;
        }
        x[i] = '\0';

        if(isdigit(x[0]))
            return (LispObject*)new_lisp_int(atoi(x));
        else
            return (LispObject*)new_symbol(x);
    }
}

void dumb_print(LispObject *obj) {
    if(obj == NULL)
        printf("NULL");
    else if(obj->type == &ConsCellType) {
        ConsCell *con = (ConsCell*)obj;
        if(con == nil)
            printf("nil");
        else {
            printf("(");
            dumb_print(con->car);
            printf(" . ");
            dumb_print(con->cdr);
            printf(")");
        }
    } else if(obj->type == &SymbolType) {
        Symbol *sym = (Symbol*)obj;
        printf("%s", sym->name);
    } else
        printf("something");
}

void repl() {
    char line[100];
    char *s;
    printf("> ");
    while(fgets(line, 100, stdin) != NULL) {
        s = line;
        if(VERBOSE)
            printf("reading...\n"); fflush(stdout);
        LispObject *r = read(&s);
        if(VERBOSE) {
            obj_print(r);
            printf("\nevaluating...\n"); fflush(stdout);
        }
        LispObject *e = eval_sub(r);
        if(VERBOSE)
            printf("printing...\n"); fflush(stdout);
        obj_print(e);
        printf("\n");
        fflush(stdout);
        if(VERBOSE) {
            printf("symbol table:\n");
            print_symbol_table();
        }
        collect_garbage();
        if(VERBOSE)
            printf("amount of memory in in alloc table: %d\n",
                   memory_in_alloc_table());
        printf("> ");
    }
}

void eval_file(char *filename) {
    FILE *f = (!strcmp(filename, "-")) ? stdin : fopen(filename, "r");
    if(f == NULL)
        error("File %s does not exist", filename);
    int size = 32000;
    char *buf = malloc(size * sizeof(char));
    int n = fread(buf, 1, size - 1, f);
    buf[n] = '\0';
    char *s = buf;
    eval_sub(read(&s));
    free(buf);
}

int main(int argc, char **argv) {
    char *file_to_eval = NULL;
    int replize = argc < 1;
    for(int i = 1; i < argc; i++) {
        if(!strcmp("-f", argv[i]))
            file_to_eval = argv[++i];
        else if(!strcmp("--verbose", argv[i]))
            VERBOSE = true;
        else if(!strcmp("-i", argv[i]))
            replize = true;
    }

    init_alloc_system();
    init_symboltable();
    register_builtin_functions();

    new_var(new_symbol("nil"), (LispObject*)nil);
    new_var(new_symbol("t"), (LispObject*)new_symbol("t"));

    nexception_points++;
    if(setjmp(exception_points[nexception_points - 1]) == 0) {
        eval_file("prelude.l");
        if(file_to_eval)
            eval_file(file_to_eval);
        else
            repl();
    } else {
        fprintf(stderr, "%s", error_string);
        printf("Stack trace:\n");
        for(int i = 0; i < call_stack->size; i++) {
            printf("  ");
            obj_print(vector_getitem(call_stack, i));
            printf("\n");
        }
        while(scopes->size > 1)
            pop_scope();
        while(call_stack->size > 1)
            vector_remove(call_stack, -1);
        if(replize)
            repl();
    }
}

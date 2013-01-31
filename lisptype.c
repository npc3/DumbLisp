#include "lisptype.h"
#include "alloc.h"
#include "error.h"
#include <string.h>

//print the LispObject obj to stdout as represented by it's str method
void obj_print(LispObject *obj) {
    const int bufsize = 4000;
    char x[bufsize];
    obj->type->str(obj, x, bufsize);
    printf("%s", x);
}

//returns obj if it is of type type, otherwise raises an exception
void *safe_cast(LispObject *obj, LispType *type) {
    if(obj->type != type)
        error("found object of type %s where %s was expected\n", obj->type->name, type->name);
    return obj;
}

/* LispObject *gc_new(LispType *type, ConsCell *args) { */
/*     LispObject *out = alloc(type->object_size); */
/*     type->init(out, args); */
/*     return out; */
/* } */

//=type=

//str method for types
int type_to_string(LispObject *obj, char *s, int n) {
    LispType *type = (LispType*)obj;
    return sncprintf(s, n, "Type %s", type->name);
}

LispType TypeType = {NULL, "TypeType", type_to_string, sizeof(LispType)};


//=symbol=

LispType SymbolType = {&TypeType, "Symbol", symbol_to_string, sizeof(Symbol)};

#define MAX_SYMBOLS 256
static Symbol symbol_cache[MAX_SYMBOLS];
static int nsymbols = 0;

//creates a new symbol represented by name, or returns the preexisting one
//in the symbol table if there is one
Symbol *new_symbol(char *name) {
    int i;
    for(i = 0; i < nsymbols; i++)
        if(!strcmp(symbol_cache[i].name, name))
            return &symbol_cache[i];
    symbol_cache[nsymbols].type = &SymbolType;
    strncpy(symbol_cache[nsymbols].name, name, MAX_SYMBOL_LEN);
    nsymbols++;
    return &symbol_cache[nsymbols - 1];
}

//str method for symbols
int symbol_to_string(LispObject *obj, char *s, int n) {
    Symbol *sym = (Symbol*)obj;
    return sncprintf(s, n, "Symbol named %s at %p", sym->name, sym);
}

//=cons=

LispType ConsCellType = {&TypeType, "ConsCell", cons_to_string, sizeof(ConsCell)};

//creats a new cons cell with car and cdr as the car and cdr
ConsCell *new_cons_cell(LispObject *car, LispObject *cdr) {
    ConsCell *out = alloc(sizeof(ConsCell));
    out->type = &ConsCellType;
    out->car = car;
    out->cdr = cdr;
    return out;
}

static ConsCell _the_real_nil = {&ConsCellType, NULL, NULL};
ConsCell *nil = &_the_real_nil;

static ConsCell _the_real_tee = {&ConsCellType, NULL, NULL};
LispObject *tee = (LispObject*)&_the_real_tee;

//str method for conscells
int cons_to_string(LispObject *obj, char *s, int n) {
    ConsCell *con = (ConsCell*)obj;
    if(obj == tee)
        return sncprintf(s, n, "t");
    else if(con == nil)
        return sncprintf(s, n, "nil");

    int used = 0;
    s[0] = '(';
    used = 1;
    used += con->car->type->str(con->car, s + used, n - used);
    used += sncprintf(s + used, n - used, " . ");
    used += con->cdr->type->str(con->cdr, s + used, n - used);
    used += sncprintf(s + used, n - used, ")");
    //printf("conscell str, used = %d, n = %d, strlen = %d, n - used = %d\n", used, n, strlen(s), n - used);
    return used;
}

//returns the length of the list starting at con
//raises an error if con is not a proper list (ie has a cell w/ a cdr that isn't a conscell)
int list_length(ConsCell *con) {
    int out = 0;
    while(con != nil) {
        con = safe_cast(con->cdr, &ConsCellType);
        out++;
    }
    return out;
}

//returns the nth item in the list starting at con
//raises an error if con is not a proper list or if it is too short for n to be a valid index
LispObject *nth_list(ConsCell *con, int n) {
    for(int i = 0; i < n; i++) {
        if(con == nil)
            error("nth list: list too short for index %d\n", n);
        con = safe_cast(con->cdr, &ConsCellType);
    }
    return con->car;
}

//=macro=

LispType MacroType = {&TypeType, "Macro", macro_to_string, sizeof(Macro)};

//creates a new macro or function
//args is a list of symbols that defines the names of the function arguments
//body is the function body
//scope_context is a pointer to the scope (a list of dicts) that the function was declared in
//is_function should be true for functions, false for macros
Macro *new_macro(ConsCell *args, ConsCell *body, ConsCell *scope_context, int is_function) {
    Macro *out = alloc(sizeof(*out));
    out->type = &MacroType;
    ConsCell *node = args;
    int i = 0;
    while(node != nil) {
        if(node->car->type != &SymbolType)
            error("Horrible error, macro argument list contains non-symbol\n");
        if(node->cdr->type != &ConsCellType)
            error("Horrible error, macro argument list is not a proper list\n");
        node = (ConsCell*)node->cdr;
        i++;
    }
    out->context = scope_context;
    out->arity = i;
    out->args = args;
    out->body = body;
    out->is_function = is_function;
    out->macro_name = NULL;
    return out;
}

//str method for functions and macros
int macro_to_string(LispObject *obj, char *s, int n) {
    Macro *mac = (Macro*)obj;
    int used;
    if(mac->macro_name != NULL)
        used = sncprintf(s, n, "%s %s",
                         mac->is_function ? "function" : "macro",
                         mac->macro_name->name);
    else
        used = sncprintf(s, n, "Anonymous function at %p", mac);
    return used;
}

//=int=

LispType LispIntType = {&TypeType, "int", lisp_int_to_string, sizeof(LispInt)};

//creates a new lisp int representing n
LispObject *new_lisp_int(int n) {
    LispInt *out = alloc(sizeof(LispInt));
    out->type = &LispIntType;
    out->n = n;
    return (LispObject*)out;
}

//str method for ints
int lisp_int_to_string(LispObject *obj, char *s, int n) {
    return sncprintf(s, n, "%d", ((LispInt*)obj)->n);
}

//returns the C int represented by the lispint obj
//raises an error if obj is not a lispint
int lisp_int_to_int(LispObject *obj) {
    LispInt *i = safe_cast(obj, &LispIntType);
    return i->n;
}

//=builtin-function=

LispType BuiltinFunctionType = {&TypeType, "Builtin Function", builtin_function_to_string, sizeof(BuiltinFunction)};

//creates a new builtin function named name, with the C function cfunc
LispObject *new_builtin_function(char *name, LispObject*(*cfunc)(ConsCell*)) {
    BuiltinFunction *out = alloc(sizeof(BuiltinFunction));
    out->type = &BuiltinFunctionType;
    out->name = name;
    out->cfunc = cfunc;
    return (LispObject*)out;
}

//str method for builtin functions
int builtin_function_to_string(LispObject *obj, char *s, int n) {
    BuiltinFunction *bf = (BuiltinFunction*)obj;
    return sncprintf(s, n, "Builtin function %s", bf->name);
}

//=vector=

LispType VectorType = {&TypeType, "vector", vector_to_string, sizeof(Vector)};

//creates a new, empty vector
LispObject *new_vector() {
    int start_size = 8;
    Vector *out = alloc(sizeof(*out));
    out->type = &VectorType;
    out->array = malloc(start_size * sizeof(LispObject*));
    out->start = start_size;
    out->end = start_size;
    out->size = 0;
    out->array_size = 8;
    return (LispObject*)out;
}

//returns the item at index i in vector v, raises an exception if i is out of range
LispObject *vector_getitem(Vector *v, int i) {
    if(i >= v->size || -i > v->size)
        error("getitem: index %d out of range in vector of size %d\n", i, v->size);
    if(i < 0)
        i += v->size;
    i = (v->start + i) % v->array_size;
    return v->array[i];
}

//sets the item at index i in vector v to obj, raises an exception if i is out of range
void vector_setitem(Vector *v, int i, LispObject *obj) {
    if(i >= v->size || -i > v->size)
        error("setitem: index %d out of range in vector of size %d\n", i, v->size);
    if(i < 0)
        i += v->size;
    i = (v->start + i) % v->array_size;
    v->array[i] = obj;
}

//swaps the items at indexes i & j in vector v, raises an exception if either i or j is out of range
void vector_swap(Vector *v, int i, int j) {
    LispObject *tmp = vector_getitem(v, i);
    vector_setitem(v, i, vector_getitem(v, j));
    vector_setitem(v, j, tmp);
}

//resizes the vector v to have space amt * its current space
void vector_resize(Vector *v, int amt) {
    int new_array_size = v->array_size * amt;
    LispObject **new_array = malloc(new_array_size * sizeof(*new_array));

    if(VERBOSE)
        printf("resizing vector at %p\n", v);
    if(new_array == NULL)
        error("out of memory\n");

    for(int j = 0; j < v->size; j++)
        new_array[j] = vector_getitem(v, j);
    free(v->array);
    v->array = new_array;
    v->array_size = new_array_size;
    v->start = v->array_size;
    v->end = v->start + v->size;
}

//inserts obj into a new space at the end of v
void vector_append(Vector *v, LispObject *obj) {
    if(v->size >= v->array_size)
        vector_resize(v, 2);
    v->array[v->end % v->array_size] = obj;
    v->end++;
    v->size++;
}

//inserts obj into v so that it has index i, pushing other elements forward
//raises an exception if i is out of range
void vector_insert(Vector *v, int i, LispObject *obj) {
    if(i >= v->size || -i > v->size)
        error("index %d out of range in vector of size %d\n", i, v->size);
    if(i < 0)
        i += v->size;

    if(v->size >= v->array_size)
        vector_resize(v, 2);

    if(i < v->size / 2) { //push back
        v->start--;
        for(int j = v->start; j < v->start + i; j++)
            v->array[j % v->array_size] = v->array[(j+1) % v->array_size];
    } else { //push forward
        v->end++;
        for(int j = v->end; j >= v->start + i; j--)
            v->array[j % v->array_size] = v->array[(j-1) % v->array_size];
    }

    v->size++;
    vector_setitem(v, i, obj);
}

//removes the element at index i from v, moving other elements backwards
//raises an exception if i is out of range
void vector_remove(Vector *v, int i) {
    if(i >= v->size || -i > v->size)
        error("index %d out of range in vector of size %d\n", i, v->size);
    if(i < 0)
        i += v->size;

    if(i < v->size / 2) {
        v->start++;
        for(int j = v->start + i; j >= v->start; j--)
            v->array[j % v->array_size] = v->array[(j-1) % v->array_size];
    } else {
        v->end--;
        for(int j = v->start + i; j < v->end; j++)
            v->array[j % v->array_size] = v->array[(j+1) % v->array_size];
    }

    v->size--;
}

//str method for vectors
int vector_to_string(LispObject *obj, char *s, int n) {
    Vector *v = (Vector*)obj;
    int i;
    int used = 1;

    if(n <= 1) return 0;
    s[0] = '[';
    for(i = 0; i < v->size; i++) {
        LispObject *x = vector_getitem(v, i);
        used += x->type->str(x, s + used, n - used);
        if(used >= n - 1)
            return n - 1;
        used += sncprintf(s + used, n - used, ", ");
        if(used >= n - 1)
            return n - 1;
    }
    if(used >= n - 1) return used;
    s[used] = ']';
    s[used + 1] = '\0';
    return used + 1;
}

//=dict=

LispType DictType = {&TypeType, "dict", dict_to_string, sizeof(Dict)};

static void dict_resize(Dict *d);
static bool dict_find_index(Dict *d, LispObject *key, int *index);

#define NPRIMES 28
static const int primes[NPRIMES] = {11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437, 102877, 205759, 411527, 823117, 1646237, 3292489, 6584983, 13169977, 26339969, 52679969, 105359939, 210719881, 421439783, 842879579, 1685759167};

//finds the index of key in the underlying array and puts it into index_out
//returns false if key is not present in the table, in which case index_out
//will be the proper index for it to be inserted into, or -1 if the table is full
static bool dict_find_index(Dict *d, LispObject *key, int *index_out) {
    int hash = (int)key; //use pointer value for hash value, hacky as hell
    int i = 0;
    *index_out = -1;
    for(;;) {
        int j = (hash + i * i) % d->array_size;
        if(i >= d->array_size)
            return false;
        if(d->keys[j] == NULL) {
            *index_out = j;
            return false;
        } if(key == d->keys[j]) { //also using reference equality!?
            *index_out = j;
            return true;
        }
        i++;
    }
}

//sets the value for key key to value in d
void dict_setitem(Dict *d, LispObject *key, LispObject *value) {
    int i;
    if(dict_find_index(d, key, &i))
        d->values[i] = value;
    else if (i >= 0) {
        d->keys[i] = key;
        d->values[i] = value;
        d->size++;
        if(d->size > d->array_size / 2)
            dict_resize(d);
    } else
        error("dict full somehow\n");
}

//returns the value for key in d
LispObject *dict_getitem(Dict *d, LispObject *key) {
    int i;
    if(!dict_find_index(d, key, &i))
        return NULL;
    return d->values[i];
}

//resizes the dict to be able to hold more elements
static void dict_resize(Dict *d) {
    if(d->primei + 1 >= NPRIMES)
        error("dict is already at maximum size\n");

    int newsize = primes[d->primei + 1];
    int oldsize = d->array_size;
    LispObject **old_keys = d->keys;
    LispObject **old_values = d->values;

    d->keys = malloc(newsize * sizeof(*d->keys));
    d->values = malloc(newsize * sizeof(*d->values));
    d->array_size = newsize;
    d->size = 0;
    d->primei++;
    memset(d->keys, 0, newsize * sizeof(*d->keys));
    memset(d->values, 0, newsize * sizeof(*d->values));

    for(int i = 0; i < oldsize; i++)
        if(old_keys[i] != NULL)
            dict_setitem(d, old_keys[i], old_values[i]);

    free(old_keys);
    free(old_values);
}

//creates a new, empty dict
LispObject *new_dict() {
    Dict *out = alloc(sizeof(*out));
    out->type = &DictType;
    out->array_size = 0;
    out->primei = -1;
    out->keys = NULL;
    out->values = NULL;
    dict_resize(out);
    return (LispObject*)out;
}

//str method for dicts
int dict_to_string(LispObject *obj, char *s, int n) {
    Dict *d = (Dict*)obj;
    int used = 0;

    used += sncprintf(s + used, n - used, "{");
    for(int i = 0; i < d->array_size; i++) {
        if(d->keys[i] == NULL)
            continue;
        if(n - used <= 0)
            return used;

        used += d->keys[i]->type->str(d->keys[i], s + used, n - used);
        used += sncprintf(s + used, n - used, " : ");
        used += d->values[i]->type->str(d->values[i], s + used, n - used);
        used += sncprintf(s + used, n - used, ", ");
    }

    used += sncprintf(s + used, n - used, "}");
    return used;
}


//=str=

LispType StrType = {&TypeType, "str", str_to_string, sizeof(Str)};

LispObject *new_str() {
    return (LispObject*)new_str_with_size(8);
}

Str *new_str_with_size(int size) {
    Str *out = alloc(sizeof(*out));
    out->type = &StrType;
    out->array = malloc(size * sizeof(char));
    out->array_size = size;
    out->size = 0;
    out->array[0] = '\0';
    return out;
}

int str_to_string(LispObject *obj, char *s, int n) {
    return sncprintf(s, n, "\"%s\"", ((Str*)obj)->array);
}

Str *str_slice(Str *s, int start, int len) {
    if(start < 0)
        start += s->size;
    if(len < 0)
        error("can't have negative length in str_slice");
    if(start + len > s->size)
        error("index error in str_slice");

    Str *out = (Str*)new_str();
    for(int i = 0; i < len; i++)
        str_append(out, s->array[start + i]);
    return out;
}

Str *str_concat(Str *a, Str *b) {
    Str *out = (Str*)new_str();
    for(int i = 0; i < a->size; i++)
        str_append(out, a->array[i]);
    for(int i = 0; i < b->size; i++)
        str_append(out, b->array[i]);
    return out;
}

void str_append(Str *s, char c) {
    if(s->size >= s->array_size - 1) {
        int newsize = s->array_size * 2;
        char *new_array = malloc(newsize * sizeof(char));
        memcpy(new_array, s->array, s->array_size);
        free(s->array);
        s->array = new_array;
        s->array_size = newsize;
    }

    s->array[s->size] = c;
    s->array[s->size + 1] = '\0';
    s->size++;
}

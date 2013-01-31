#include "alloc.h"
#include "lisptype.h"
#include "symboltable.h"

#include "builtins.h"

typedef struct AllocNode_S {
    struct AllocNode_S *next;
    void *value;
    int marked;
    size_t size;
} AllocNode;

#define ALLOC_ROOT_SIZE 1024
static AllocNode *alloc_root[ALLOC_ROOT_SIZE];
static size_t total_memory_use = 0;

//initialize the allocation system
void init_alloc_system() {
    int i;
    for(i = 0; i < ALLOC_ROOT_SIZE; i++)
        alloc_root[i] = NULL;
}

//allocate a new block of memory of size size
void *alloc(size_t size) {
    void *value = malloc(size);
    AllocNode *node = malloc(sizeof(AllocNode));
    node->value = value;
    node->size = size;
    int i = ((int)value) % ALLOC_ROOT_SIZE;
    node->next = alloc_root[i];
    alloc_root[i] = node;
    total_memory_use += size + sizeof(AllocNode);
    return value;
}

//find the alloc node holding the memory pointer value
//the node before it is put into prev_out, or NULL if there is none
static AllocNode *find_alloc_node(void *value, AllocNode **prev_out) {
    int i = ((int)value) % ALLOC_ROOT_SIZE;
    AllocNode *prev = NULL;
    AllocNode *node = alloc_root[i];
    while(node != NULL) {
        if(node->value == value)
            break;
        prev = node;
        node = node->next;
    }
    if(prev_out != NULL)
        *prev_out = prev;
    return node;
}

//calculates the amount of memory in the alloc table
size_t memory_in_alloc_table() {
    size_t out = 0;
    int i;
    for(i = 0; i < ALLOC_ROOT_SIZE; i++) {
        AllocNode *node = alloc_root[i];
        while(node != NULL) {
            out += node->size + sizeof(AllocNode);
            node = node->next;
        }
    }
    return out;
}

//marks an object and all it's referenced objects (currently hard-coded code) as being live
static void gc_mark(LispObject *obj) {
    AllocNode *an = find_alloc_node(obj, NULL);
    if(an == NULL) {
        if(obj->type != &SymbolType && obj != (LispObject*)nil)
            fprintf(stderr,
                    "Rather horrid error, object at %p of type %s not in alloc_table\n",
                    obj,
                    obj->type->name);
        return;
    }
    if(an->marked)
        return;
    an->marked = true;
    
    if(obj->type == &ConsCellType) {
        ConsCell *con = (ConsCell*)obj;
        gc_mark(con->car);
        gc_mark(con->cdr);
    } else if(obj->type == &MacroType) {
        Macro *mac = (Macro*)obj;
        gc_mark((LispObject*)mac->args);
        gc_mark((LispObject*)mac->body);
        gc_mark((LispObject*)mac->context);
    } else if(obj->type == &VectorType) {
        Vector *v = (Vector*)obj;
        for(int i = 0; i < v->size; i++)
            gc_mark(vector_getitem(v, i));
    } else if(obj->type == &DictType) {
        Dict *d = (Dict*)obj;
        for(int i = 0; i < d->array_size; i++)
            if(d->keys[i] != NULL) {
                gc_mark(d->keys[i]);
                gc_mark(d->values[i]);
            }
    }
}

//deallocates dead objects and removes their entries from the alloc table
void collect_garbage() {
    int i;
    for(i = 0; i < ALLOC_ROOT_SIZE; i++) {
        AllocNode *node = alloc_root[i];
        while(node != NULL) {
            node->marked = false;
            node = node->next;
        }
    }

    gc_mark((LispObject*)scopes);

    gc_mark((LispObject*)call_stack);

    for(i = 0; i < ALLOC_ROOT_SIZE; i++) {
        AllocNode *prev = NULL;
        AllocNode *node = alloc_root[i];
        AllocNode *tmp;
        while(node != NULL) {
            if(!node->marked) {
                if(ALLOC_VERBOSE) {
                    printf("garbage collecting object of type %s at %p \n",
                           ((LispObject*)node->value)->type->name,
                           node->value);
                    printf("total mem: %d, amt in alloc table: %d, difference: %d\n",
                           total_memory_use,
                           memory_in_alloc_table(),
                           total_memory_use - memory_in_alloc_table());
                }
  
                if(prev == NULL)
                    alloc_root[i] = node->next;
                else
                    prev->next = node->next;

                total_memory_use -= node->size + sizeof(AllocNode);

                tmp = node->next;
                free(node->value);
                free(node);
                node = tmp;
            } else {
                prev = node;
                node = node->next;
            }
        }
    }
}

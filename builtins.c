#include "builtins.h"
#include "symboltable.h"
#include "error.h"


Vector *call_stack;


LispObject *eval(ConsCell *args) {
    //args is a one elem list whose elem is a form that will be evaluated and the result evaluated
    if(list_length(args) != 1)
        error("Horrible error, wrong number of arguments to eval\n");
    return eval_sub(eval_sub(args->car));
}

LispObject *eval_sub(LispObject *obj) {
    //obj is the object to be evaluated
    LispObject *out;

    if(obj->type == &SymbolType)
        out = get_var((Symbol*)obj);
    else if(obj->type == &ConsCellType) {
        ConsCell *con = (ConsCell*)obj;
        if(con == nil)
            out = (LispObject*)nil;
        else
            out = apply_sub(con->car, (ConsCell*)con->cdr);
    } else
        out = obj;
    return out;
}

LispObject *apply(ConsCell *args) {
    //args is a list whose 1st elem is an expression that will evaluate to the function to be applied
    //and whose 2nd elem is an expression that will evaluate to the arguments to the function
    if(list_length(args) != 2)
        error("Horrible error, wrong number of arguments to apply");

    LispObject *function = args->car;
    ConsCell *function_arguments = (ConsCell*)eval_sub(((ConsCell*)args->cdr)->car);
    return apply_sub(function, function_arguments);
}

LispObject *apply_sub(LispObject *function, ConsCell *function_arguments) {
    //function is an expression that will evaluate to the function to be applied
    //function_arguments is a list of elems that will be passed as arguments to function
    //(without being evaluated)
    LispObject *out;

    if(function_arguments->type != &ConsCellType)
        error("Horrible error, 2nd argument of apply is not a list");

    function = eval_sub(function);

    if(VERBOSE) {
        printf("applying "); obj_print(function); printf("\n");
        //printf(" to "); obj_print((LispObject*)function_arguments);
    }

    vector_append(call_stack, function);

    //builtin function
    if(function->type == &BuiltinFunctionType) {
        BuiltinFunction *bf = (BuiltinFunction*)function;
        out = bf->cfunc(function_arguments);
    } else {
        //macro or function
        Macro *func = safe_cast(function, &MacroType);

        Dict *new_scope = (Dict*)new_dict();
        LispObject *rest_of_context = func->is_function ?
            (LispObject*)func->context :
            vector_getitem(scopes, -1);
        ConsCell *new_scope_context = new_cons_cell((LispObject*)new_scope, rest_of_context);
        ConsCell *namecell = func->args;
        ConsCell *valcell = function_arguments; //check this?
        while(namecell != nil) {
            if(valcell == nil)
                error("Horrible error, not enough arguments to function\n");
            LispObject *val = func->is_function ? eval_sub(valcell->car) : valcell->car;
            //new_var((Symbol*)namecell->car, val);
            dict_setitem(new_scope, namecell->car, val); //kinda hacky, this
            namecell = (ConsCell*)namecell->cdr;
            valcell = (ConsCell*)valcell->cdr;
        }
        if(valcell != nil)
            error("Horrible error, too many arguments to function\n");

        push_scope(new_scope_context);
        out = do_(func->body);
        pop_scope();
        if(!func->is_function)
            out = eval_sub(out);
    }
    if(VERBOSE) {
        printf(" and receiving "); obj_print(out); printf("\n");
    }

    vector_remove(call_stack, -1);

    return out;
}

LispObject *do_(ConsCell *args) {
    //args is a list of which each element will be evaluated and the last result returned
    //nil is returned if args is empty
    LispObject *out = (LispObject*)nil;
    while(args != nil) {
        out = eval_sub(args->car);
        args = (ConsCell*)args->cdr;
    }
    return out;
}

LispObject *quote(ConsCell *args) {
    //args is a one elem list whose elem is returned unevaluated
    if(list_length(args) != 1)
        error("Horrible error, wrong number of arguments to quote\n");
    return args->car;
}

LispObject *cons(ConsCell *args) {
    //args is a 2-elem list. both elems are evaluated, and a new cons cell is returned with the
    //1st result as the car and the 2nd as the cdr
    if(list_length(args) != 2)
        error("Horrible error, wrong number of arguments to cons\n");
    return (LispObject*)new_cons_cell(eval_sub(args->car), eval_sub(((ConsCell*)args->cdr)->car));
}

LispObject *list(ConsCell *args) {
    //args is a list whose elems are evaluated and returned as a new list
    if(args == nil)
        return (LispObject*)nil;
    ConsCell *out = new_cons_cell((LispObject*)nil, (LispObject*)nil);
    ConsCell *node = out;
    for(;;) {
        node->car = eval_sub(args->car);
        args = (ConsCell*)args->cdr;
        if(args == nil)
            return (LispObject*)out;
        node->cdr = (LispObject*)new_cons_cell((LispObject*)nil, (LispObject*)nil);
        node = (ConsCell*)node->cdr;
    }
}

LispObject *macro(ConsCell *args) {
    //args is a list whose first element is a list of symbols for the argument names
    //the rest is the body of the macro
    if(args == nil)
        error("Horrible error, not enough arguments to macro\n");
    if(args->car->type != &ConsCellType)
        error("Horrible error, first argument to macro is not a list\n");

    Macro *out = new_macro((ConsCell*)args->car,
                           (ConsCell*)args->cdr,
                           nil,
                           false);
    return (LispObject*)out;
}

LispObject *fn(ConsCell *args) {
    //args is a list whose first element is a list of symbols for the argument names
    //the rest is the body of the function
    if(args == nil)
        error("Horrible error, not enough arguments to fn\n");
    if(args->car->type != &ConsCellType)
        error("Horrible error, first argument to fn is not a list\n");

    Macro *out = new_macro((ConsCell*)args->car,
                           (ConsCell*)args->cdr,
                           (ConsCell*)vector_getitem(scopes, -1),
                           true);
    return (LispObject*)out;

}

LispObject *def(ConsCell *args) {
    //args is a 2-elem list, the first of which is a symbol, the second is a form which will be
    //evaluated and the first elems entry in the symbol table set to it
    if(list_length(args) != 2)
        error("Horrible error, wrong number of arguments to def\n");
    Symbol *sym = safe_cast(args->car, &SymbolType);
    LispObject *val = nth_list(args, 1);
    val = eval_sub(val);

    new_var(sym, val);

    if(val->type == &MacroType) {
        Macro *mac = (Macro*)val;
        if(mac->macro_name == NULL)
            mac->macro_name = sym;
    }

    return val;
}

LispObject *car(ConsCell *args) {
    //args is a one elem list consisting of a form that will be evaluated,
    //and the result's car returned
    if(list_length(args) != 1)
        error("Horrible error, wrong number of arguments to car");
    ConsCell *obj = (ConsCell*)eval_sub(args->car);
    if(obj->type != &ConsCellType)
        error("Horrible error, argument to car is not a list");
    return (LispObject*) obj->car;
}

LispObject *cdr(ConsCell *args) {
    //args is a one elem list consisting of a form that will be evaluated,
    //and the result's cdr returned
    if(list_length(args) != 1)
        error("Horrible error, wrong number of arguments to cdr");
    ConsCell *obj = (ConsCell*)eval_sub(args->car);
    if(obj->type != &ConsCellType)
        error("Horrible error, argument to cdr is not a list");
    return (LispObject*) obj->cdr;
}

LispObject *if_(ConsCell *args) {
    //args is a list of 3 elements. the first is evaluated, and if the result is not nil,
    //the second is evaluated and returned, otherwise the third is
    if(list_length(args) != 3)
        error("Horrible error, wrong number of arguments to if");
    if(eval_sub(args->car) != (LispObject*)nil)
        return eval_sub(nth_list(args, 1));
    else
        return eval_sub(nth_list(args, 2));
}

LispObject *equals(ConsCell *args) {
    //args is a list of 2 elements which are evaluated, and if they are equal
    //t is returned, else nil
    if(list_length(args) != 2)
        error("Horrible error, wrong number of arguments to =");
    return equals_sub(nth_list(args, 0), nth_list(args, 1));
}

LispObject *equals_sub(LispObject *a, LispObject *b) {
    //if a and b are equal (ints representing the same number or the same object)
    //t is returned, else nil
    a = eval_sub(a);
    b = eval_sub(b);
    if(a->type != b->type)
        return (LispObject*)nil;
    else if(a->type == &LispIntType)
        return ((LispInt*)a)->n == ((LispInt*)b)->n ? tee : (LispObject*)nil;
    else
        return a == b ? tee : (LispObject*)nil; //reference equality i guess?
}

LispObject *plus(ConsCell *args) {
    //all the elements in args are evaluated, and their sums returned
    //if args is empty, 0 is returned
    //raises an exception if any element does not evaluate to an int
    int out = 0;
    while(args != nil) {
        LispObject *val = eval_sub(args->car);
        out += lisp_int_to_int(val);
        args = (ConsCell*)args->cdr;
    }
    return new_lisp_int(out);
}

LispObject *minus(ConsCell *args) {
    //all the elements in args are evaluated
    //if args is empty, 0 is returned. if args has only 1 element, its negation is returned
    //else, the first element minus the sum of the rest is returned
    //raises an exception if any element does not evaluate to an int
    int out = 0;
    int first = true;
    int morethanone = false;
    while(args != nil) {
        int val = lisp_int_to_int(eval_sub(args->car));
        if(first) {
            out = val;
            first = false;
        } else {
            out -= val;
            morethanone = true;
        }
        args = (ConsCell*)args->cdr;
    }
    if(!morethanone)
        out = -out;
    return new_lisp_int(out);
}

LispObject *print(ConsCell *args) {
    //all the elements of args are evaluated, and their representation (as defined by
    //their str methods) are printed to stdout
    LispObject *obj = (LispObject*)nil;
    while(args != nil) {
        obj = eval_sub(args->car);
        obj_print(obj);
        printf(" ");
        args = (ConsCell*)args->cdr;
    }
    printf("\n");
    return obj;
}

LispObject *while_(ConsCell *args) {
    //the first element of args is evaluated, and then the rest in a do block.
    //continues in a loop until the first element evaluates to nil
    LispObject *out = (LispObject*)nil;
    while(eval_sub(args->car) != (LispObject*)nil)
        out = do_((ConsCell*)args->cdr);
    return out;
}

LispObject *set(ConsCell *args) {
    //the first element of args is a symbol with a value in the current symbol table
    //the second element is evaluated and the result put into the symbol table
    //raises an exception if the first element is not a symbol, if it does not have
    //an entry in the symbol table, or if there are not exactly 2 arguments
    if(list_length(args) != 2)
        error("Horrible error, wrong number of arguments to set\n");
    LispObject* val = nth_list(args, 1);
    val = eval_sub(val);
    set_var((Symbol*)safe_cast(args->car, &SymbolType), val);
    return val;
}

LispObject *try_catch(ConsCell *args) {
    //the first element of args is evaluated, and if an exception is raised during evaluation
    //the second element is evaluated and execution continues normally
    //raises an exception if there is not exactly 2 arguments
    if(list_length(args) != 2)
        error("Horrible error, wrong number of arguments to try-catch");

    int my_nscopes = scopes->size;
    int my_call_stack_size = call_stack->size;
    nexception_points++;
    if(setjmp(exception_points[nexception_points - 1]) == 0) {
        return eval_sub(args->car);
        nexception_points--;
    } else {
        if(VERBOSE)
            printf("exception point number %d being called\n", nexception_points - 1);
        nexception_points--;
        while(scopes->size > my_nscopes)
            pop_scope();
        while(call_stack->size > my_call_stack_size)
            vector_remove(call_stack, -1);
        return eval_sub(nth_list(args, 1));
    }
}

LispObject *show_symbol_table(ConsCell *args) {
    //prints the current contents of the symbol table to stdout
    print_symbol_table();
    return (LispObject*)nil;
}

LispObject *vector(ConsCell *args) {
    //args is a list whose elems are evaluated and put into the vector
    Vector *out = (Vector*)new_vector();
    while(args != nil) {
        vector_append(out, eval_sub(args->car));
        args = (ConsCell*)args->cdr;
    }
    return (LispObject*)out;
}

LispObject *nth(ConsCell *args) {
    //1st elem evaluates to a vector, 2nd to int
    Vector *v = safe_cast(eval_sub(args->car), &VectorType);
    int i = lisp_int_to_int(eval_sub(nth_list(args, 1)));
    return vector_getitem(v, i);
}

LispObject *insert(ConsCell *args) {
    //1st evaluates to a vector, 2nd to int, 3rd to any object
    Vector *v = safe_cast(eval_sub(args->car), &VectorType);
    int i = lisp_int_to_int(eval_sub(nth_list(args, 1)));
    LispObject *obj = eval_sub(nth_list(args, 2));
    vector_insert(v, i, obj);
    return (LispObject*)v;
}

LispObject *append(ConsCell *args) {
    //1st evaluates to a vector, 2nd to obj which is appended to 1st
    Vector *v = safe_cast(eval_sub(args->car), &VectorType);
    vector_append(v, eval_sub(nth_list(args, 1)));
    return (LispObject*)v;
}

LispObject *dict(ConsCell *args) {
    //creates a new dict, empty if no args
    //otherwise takes as arguments two elements which are evaluated
    //one is a list of keys and the other a list of corresponding values
    //raises an exception if there are not exactly 0 or 2 arguments, if either
    //argument is not a proper list, or if the lists are not of the same length
    Dict *out = (Dict*)new_dict();
    int len = list_length(args);
    if(len == 2) {
        ConsCell *keys = safe_cast(eval_sub(nth_list(args, 0)), &ConsCellType);
        ConsCell *values = safe_cast(eval_sub(nth_list(args, 1)), &ConsCellType);

        while(keys != nil) {
            if(values == nil)
                error("mismatch in length of argument lists to dict\n");
            dict_setitem(out, keys->car, values->car);
            keys = safe_cast(keys->cdr, &ConsCellType);
            values = safe_cast(values->cdr, &ConsCellType);
        }
        if(values != nil)
            error("mismatch in length of argument lists to dict\n");
    } else if(len != 0)
        error("wrong number of arguments to dict\n");
    return (LispObject*)out;
}

LispObject *getitem(ConsCell *args) {
    //1st is evaluated to be a dict, 2nd to key, value of key in dict is returned
    //raises exception if 1st doesn't evaluate to dict
    LispObject *out = dict_getitem(safe_cast(eval_sub(nth_list(args, 0)), &DictType),
                                   eval_sub(nth_list(args, 1)));
    if(out == NULL)
        error("item not found in dict\n");
    return out;
}

LispObject *setitem(ConsCell *args) {
    //1st is evaluated to be a dict, 2nd to key, third to value
    //raises exception if 1st doesn't evaluate to dict
    //returns modified dict
    Dict *out = safe_cast(eval_sub(nth_list(args, 0)), &DictType);
    LispObject *key = eval_sub(nth_list(args, 1));
    LispObject *value = eval_sub(nth_list(args, 2));
    dict_setitem(out, key, value);
    return (LispObject*)out;
}

LispObject *exit_(ConsCell *args) {
    //exits program with status code of evaluation of 1st argument, defaults to 0 if no arguments
    int status = 0;
    if(args != nil)
        status = lisp_int_to_int(eval_sub(args->car));
    exit(status);
}

LispObject *slice(ConsCell *args) {
    Str *s = safe_cast(eval_sub(nth_list(args, 0)), &StrType);
    int start = lisp_int_to_int(eval_sub(nth_list(args, 1)));
    int len = lisp_int_to_int(eval_sub(nth_list(args, 2)));
    return (LispObject*)str_slice(s, start, len);
}

LispObject *concat(ConsCell *args) {
    int len = list_length(args);
    if(len == 0)
        return new_str();
    else if(len == 1)
        return safe_cast(eval_sub(nth_list(args, 0)), &StrType);
    else {
        Str *out = safe_cast(eval_sub(nth_list(args, 0)), &StrType);
        for(int i = 1; i < len; i++)
            out = str_concat(out, safe_cast(eval_sub(nth_list(args, i)), &StrType));
        return (LispObject*)out;
    }
}


void register_builtin_functions() {
    //creates the builtin function objects and puts them into the symbol table
    #define NBUILTINS 30
    char *names[NBUILTINS] = {"eval", "apply", "do", "quote", "cons", "list", "macro",
                              "fn", "def", "car", "cdr", "if", "=", "+", "-",
                              "print", "while", "set", "try-catch", "show-symbol-table",
                              "vector", "nth", "insert", "append",
                              "dict", "getitem", "setitem",
                              "exit",
                              "slice", "concat"};
    LispObject *(*funcs[NBUILTINS])(ConsCell *) = {eval, apply, do_, quote, cons, list, macro,
                                                   fn, def, car, cdr, if_, equals, plus, minus,
                                                   print, while_, set, try_catch, show_symbol_table,
                                                   vector, nth, insert, append,
                                                   dict, getitem, setitem,
                                                   exit_,
                                                   slice, concat};
    int i;
    for(i = 0; i < NBUILTINS; i++)
        new_var(new_symbol(names[i]), new_builtin_function(names[i], funcs[i]));

    call_stack = (Vector*)new_vector();
}

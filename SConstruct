import sys

def yes(*args, **kwargs):
    return True

if '-b' in sys.argv:
    Decider(yes)
    
files = Split('alloc.c main.c error.c symboltable.c builtins.c lisptype.c common.c')

env = Environment(CFLAGS='-g --std=c99 -Wall')
prog = env.Program('lisp', files, CPPPATH = '.')
env.NoClean(prog)

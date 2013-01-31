#!/bin/bash

TESTSDIR=tests

for DIR in $TESTSDIR/*; do
    echo "===testing $DIR==="
    ./lisp -f $DIR/program > $TESTSDIR/actual_result
    DIFF=$(diff -w $DIR/output $TESTSDIR/actual_result)
    if (($? != 0)); then
        echo "  Test FAILURE! Diff:"
        echo $DIFF
    else
        echo "Test Success"
    fi
done

rm $TESTSDIR/actual_result

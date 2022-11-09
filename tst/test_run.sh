#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    testdir="$1TESTRUN"
    echo "REPLAYING $1, TESTDIR $testdir"
    cp -r $1 $testdir 
    cd $testdir
    rm -rf screenshot*
    cd ../..
    build/mmfm -r res -v -p $testdir -d $testdir    
    echo "REPLAY FINISHED, DIFFING"
    diff -r $1 $testdir
fi


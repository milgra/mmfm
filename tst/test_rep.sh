#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    testdir="$1_TESTRUN"
    savedir="$1_TESTRUN/record"
    echo "REPLAYING $1, TESTDIR $testdir"
    rm -rf $testdir
    cp -r $1 $testdir 
    cd $savedir
    rm -rf *.kvl
    rm -rf screenshot*
    cd ../../..
    build/mmfm -r res -v -p $savedir -d $testdir -c $savedir    
    echo "REPLAY FINISHED, DIFFING"
    diff -r $1 $testdir
fi


#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    basedir="$1_base"
    testdir="$1_test"
    savedir="$1_test/record"
    masterdir="$1_master"
    echo "REPLAYING $1, TESTDIR $testdir"
    # cleanup
    rm -rf $testdir
    cp -r $basedir $testdir 
    cd $savedir
    rm -rf *.kvl
    rm -rf screenshot*
    cd ../../..
    build/mmfm -r res -v -p $savedir -d $testdir -c $savedir    
    echo "REPLAY FINISHED, DIFFING"
    diff -r $masterdir $testdir
fi


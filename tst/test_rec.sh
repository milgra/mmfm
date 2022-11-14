#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    # move stuff to testdir so all path remain the same during testing and recording
    testdir="$1_TESTRUN"
    rm -rf $testdir
    cp -r $1 $testdir 
    echo "(RE-)RECORDING $1"
    cd $testdir
    rm -rf screenshot*
    rm -rf session.rec
    cd ../..
    build/mmfm -r res -v -s $testdir -d $testdir
    echo "RECORDING FINISHED"
    rm -rf $1
    cp -r $testdir $1
fi

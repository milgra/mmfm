#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    # move stuff to testdir so all path remain the same during testing and recording
    testdir="$1_TESTRUN"
    savedir="$1_TESTRUN/record"
    rm -rf $testdir
    cp -r $1 $testdir 
    echo "(RE-)RECORDING $1"
    cd $savedir
    rm -rf *.kvl
    rm -rf screenshot*
    rm -rf session.rec
    cd ../../..
    build/mmfm -r res -v -s $savedir -d $testdir -c $savedir
    echo "RECORDING FINISHED"
    rm -rf $1
    cp -r $testdir $1
fi

#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    # move stuff to testdir so all path remain the same during testing and recording
    basedir="tst/test_files"
    testdir="$1_test"
    savedir="$1_test/record"
    masterdir="$1_master"
    echo "MAKING DIR $savedir"
    rm -rf $testdir
    cp -r $basedir $testdir 
    mkdir -p $savedir
    echo "(RE-)RECORDING $1"
    build/mmfm -r res -v -s $savedir -d $testdir -c $savedir -f 1200x800
    echo "RECORDING FINISHED"
    rm -rf $masterdir
    cp -r $testdir $masterdir
    rm -rf $testdir
fi

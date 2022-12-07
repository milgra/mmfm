#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    basedir="tst/test_files"
    testdir="$1_test"
    savedir="$1_test/record"
    masterdir="$1_master"
    echo "REPLAYING $1, BASEDIR $basedir TESTDIR $testdir SAVEDIR $savedir MASTERDIR $masterdir EXE $2"
    # cleanup
    rm -rf $testdir
    cp -r $basedir $testdir 
    # copy session record
    mkdir -p $savedir
    cp "$masterdir/record/session.rec" "$savedir/"
    cd $savedir
    rm -rf *.kvl
    rm -rf screenshot*
    cd ../../..
    echo "COMMAND: $2 -r res -v -p $savedir -d $testdir -c $savedir -f 1200x800"
    $2 -r res -v -p $savedir -d $testdir -c $savedir -f 1200x800    
    echo "REPLAY FINISHED, DIFFING"
    diff -r $masterdir $testdir
    
    error=$?
    if [ $error -eq 0 ]
    then
	echo "No differences found between master and result images"
	exit 0
    elif [ $error -eq 1 ]
    then
	echo "Differences found between master and result images"
	exit 1
    else
	echo "Differences found between master and result images"
	exit 1
    fi

fi


#!/bin/bash

# usage of script :
# ./tst/test_rep.sh build/mmfm tst/test_files tst/file_info_master res 

# $1 - executable
# $2 - test files
# $3 - test folder name
# $4 - resources folder

# to replay a session :
# cp -r "tst/test_files" "tst/file_info_master/files_master" 
# mmfm --replay="tst/file_info_master/session.rec" \
#      --library="tst/file_info_master/library_test" \
#      --organize

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    
    source_library=$2
    master_library="$3/files_master"
    session_library="$3/files_test"
    session_file="$3/session.rec"

    rm -rf $session_library
    cp -r $source_library $session_library 

    echo "REPLAYING $3"    
    echo "COMMAND: $1 --resources=$4 --replay=$session_file --directory=$session_library -frame=1200x800"

    $1 --resources=$4 \
       --replay=$session_file \
       --directory=$session_library \
       --frame=1200x800 \
       -v
    
    echo "REPLAYING FINISHED"

    # remove empty dirs from both directiories to make diff work

    find $master_library -empty -type d -delete
    find $session_library -empty -type d -delete

    diff -r $master_library $session_library
    
    error=$?
    if [ $error -eq 0 ]
    then
	echo "No differences found between master and test folders"
	exit 0
    elif [ $error -eq 1 ]
    then
	echo "Differences found between master and result folders"
	exit 1
    else
	echo "Differences found between master and result folders"
	exit 1
    fi

fi

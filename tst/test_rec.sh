#!/bin/bash

# usage of script :
# ./tst/test_rec.sh tst/file_info_master build/mmfm

# $1 - test folder name
# $2 - executable

# to record a session :
# cp -r "tst/test_files" "tst/file_info_master/files_master" 
# vmp --record="tst/file_info_master/session.rec" \
#     --directory="tst/file_info_master/files_master" \

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    
    source_library="tst/test_files"
    session_library="$1/files_master"
    session_file="$1/session.rec"

    rm -rf $session_library
    cp -r $source_library $session_library 

    echo "RECORDING $1"    
    echo "COMMAND: $1 -v --resources=res --record=$session_file --directory=$session_library -frame=1200x800"

    $2 --resources=res \
       --record=$session_file \
       --directory=$session_library \
       --frame=1200x800 \
       -v
    
    echo "RECORDING FINISHED"

fi

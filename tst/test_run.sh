#!/bin/bash

exe="$1/mmfm"

sh tst/test_rep.sh tst/ui_file $exe

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

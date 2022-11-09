#!/bin/bash

if [ $# -eq 0 ]; then
    echo "PLEASE PROVIDE TEST FOLDER"
else
    echo "(RE-)RECORDING $1"
    cd $1
    rm -rf screenshot*
    rm -rf session.rec
    cd ../..
    build/mmfm -r res -v -s $1 -d $1
    echo "RECORDING FINISHED"
fi

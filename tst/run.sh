#!/bin/bash

# usage of script :
# ./tst/test_run.sh BUILD_PATH=build PROJECT_ROOT=.

BUILD_PATH=build
PROJECT_ROOT=.

# parse arguments

for ARGUMENT in "$@"
do
   KEY=$(echo $ARGUMENT | cut -f1 -d=)
   KEY_LENGTH=${#KEY}
   VALUE="${ARGUMENT:$KEY_LENGTH+1}"
   export "$KEY"="$VALUE"
done

echo "TEST_RUN BUILD PATH = $BUILD_PATH PROJECT_ROOT = $PROJECT_ROOT"

EXECUTABLE="$BUILD_PATH/mmfm"
RESOURCES="$PROJECT_ROOT/res"

for FOLDER in $PROJECT_ROOT/tst/*test; do
    echo "tst/replay.sh EXECUTABLE=$EXECUTABLE TST_FOLDER=tst TEST_NAME=$(basename $FOLDER)"

    tst/replay.sh \
       EXECUTABLE=$EXECUTABLE \
       TEST_FOLDER=tst \
       TEST_NAME=$(basename $FOLDER)
    
    error=$?
    if [ $error -eq 0 ]
    then
	echo "No differences found between master and result images"
    elif [ $error -eq 1 ]
    then
	echo "Differences found between master and result images"
	exit 1
    else
	echo "Differences found between master and result images"
	exit 1
    fi
    
done

exit 0

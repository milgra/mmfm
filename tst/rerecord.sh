#!/bin/bash

# usage of script :
# rerecord.sh TEST_NAME=generic_test TEST_FOLDER=tst EXECUTABLE=build/mmfm RESOURCES=res FRAME=1200x800 FONT=svg/terminus.otb

TEST_NAME=generic_test
TEST_FOLDER=tst
EXECUTABLE=build/mmfm
RESOURCES=res
FRAME=1200x800
FONT=svg/terminus.otb

# parse arguments

for ARGUMENT in "$@"
do
   KEY=$(echo $ARGUMENT | cut -f1 -d=)
   KEY_LENGTH=${#KEY}
   VALUE="${ARGUMENT:$KEY_LENGTH+1}"
   export "$KEY"="$VALUE"
done

SOURCE_LIB="$TEST_FOLDER/test_files"
TARGET_LIB="$TEST_FOLDER/$TEST_NAME/files_master"
SESSION="$TEST_FOLDER/$TEST_NAME/session.rec"

rm -rf $TARGET_LIB
cp -r $SOURCE_LIB $TARGET_LIB 

echo "RECORDING $TEST_NAME"
echo "COMMAND: $EXECUTABLE \
 -v \
--resources=$RESOURCES \
--record=$SESSION \
--directory=$TARGET_LIB \
--frame=1200x800 \
--font=$FONT \
-v"

$EXECUTABLE \
    --resources=$RESOURCES \
   --replay=$SESSION \
   --directory=$TARGET_LIB \
   --frame=1200x800 \
   --font=$FONT \
   -v

echo "RECORDING FINISHED"

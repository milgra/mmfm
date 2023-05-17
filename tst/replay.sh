#!/bin/bash

# usage of script :
# replay.sh TEST_NAME=generic_test TEST_FOLDER=tst EXECUTABLE=build/mmfm RESOURCES=res FRAME=1200x800 FONT=svg/terminus.otb

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
MASTER_LIB="$TEST_FOLDER/$TEST_NAME/files_master"
TARGET_LIB="$TEST_FOLDER/$TEST_NAME/files_test"
SESSION="$TEST_FOLDER/$TEST_NAME/session.rec"

rm -rf $TARGET_LIB
cp -r $SOURCE_LIB $TARGET_LIB 

echo "REPLAYING $TEST_NAME"    
echo "COMMAND: $EXECUTABLE \
--resources=$RESOURCES \
--replay=$SESSION \
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

echo "REPLAYING FINISHED"

# remove empty dirs from both directiories to make diff work

find $MASTER_LIB -empty -type d -delete
find $TARGET_LIB -empty -type d -delete

diff -r $MASTER_LIB $TARGET_LIB

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

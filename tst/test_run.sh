#!/bin/bash

echo "TEST_RUN $1 $2"

exe="$1/mmfm"
res="$2/res"
src="$2/tst/test_files"

for folder in $2/tst/*test; do
    echo "FOLDER" $folder
    sh tst/test_rep.sh $exe $src $folder $res
    
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

#!/bin/bash

# first copy start folder structure to result folder

if [ $1 == 0 ];
then
echo "COPYING start TO test"
rm -r tst/test
rm -r tst/result
cp -r tst/start tst/test
fi

# execute record session on result folder

echo "STARTING RECORDING SESSION" $1

cnt=$1
res="y"

while [ $res = "y" ]; do

    res_path="../res"
    cfg_path="../tst/test/cfg"
    ses_path="../tst/session$cnt.rec"
    frm_size="1000x900"
    
    ((cnt+=1))
    
    bin/zenmusic -r $res_path -c $cfg_path -s $ses_path -f $frm_size

    echo "Record another session? y/n"

    read res

done

echo "RENAMING test to result"

mv tst/test tst/result

echo "RECORDING FINISHED"

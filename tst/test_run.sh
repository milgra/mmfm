#!/bin/bash

# first copy start folder structure to test folder

echo "COPYING start TO test"

rm -r tst/test
cp -r tst/start tst/test

# execute replay session on test folder

echo "STARTING TEST SESSION"

cnt=0

while true; do
    
    res_path="../res"
    cfg_path="../tst/test/cfg"
    ses_path="../tst/session$cnt.rec"
    abs_path="tst/session$cnt.rec"
    frm_size="1000x900"

    ((cnt+=1))

    if test -f $abs_path; then
	build/mmfm -r $res_path -c $cfg_path -p $ses_path -f $frm_size
    else
	break
    fi
    
done

# compare result and test folders

echo "TEST RESULTS"

diff -r tst/result tst/test

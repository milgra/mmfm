# Multimedia File Manager test protocol

Before creating a pull request first check for leaks around your modification first with the built-in leak checker ( automatically executed on exit in dev mode ) and with valgrind(freebsd) or valgrind/address sanitizer(linux)
Then create a release build with meson build --buildtype=debug
Then run all test sessions ( tst/test_run.sh )
Note : test sessions should be run under a floating window manager because of fixed window dimensions.
Check the diff log after all test sessions finished. Only the dates in zemusic.kvl should differ, media files, library structure and screenshots shouldn't differ.

---

How to run a test session :

bash tst/test_run,sh

or

bulid/mmfm -r ../res -c ../tst/test/cfg -p ../tst/session.rec  

How to record a test session :

bash tst/test_rec.sh

or

build/mmfm -r ../res -c ../tst/test/cfg -s ../tst/session.rec

How to debug session recording :

lldb -- build/mmfm -f 800x600 -r ../res -c ../tst/test/cfg -s ../tst/session1.rec

How to check for memory leaks :

A dev build will auto-check zc managed memory blocks for leaks, reports it in the last stdout line.

For non-zc managed memory use address sanitizer :

CC=clang meson build --buildtype=debug -Db_sanitize=address -Db_lundef=false

A dev build will report detected leaks after closing the program.

TODO

## Protocol

If you are re-recording the main test session, follow this protocol.
If you add a new feature please add it to a proper place in the protocol.
If you see SCREENSHOT take a screenshot by pressing PRTINSCREEN button


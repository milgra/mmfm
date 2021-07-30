# Zen Music test protocol

Before creating a pull request first check for leaks around your modification first with the built-in leak checker ( automatically executed on exit in dev mode ) and with valgrind(freebsd) or valgrind/address sanitizer(linux)
Then create a release build with gmake/make rel.
Then run all test sessions with gmake/make runtest.
Note : test sessions should be run under a floating window manager because of fixed window dimensions.
Check the diff log after all test sessions finished. Only the dates in zemusic.kvl should differ, media files, library structure and screenshots shouldn't differ.

---

How to run a test session :

gmake/make runtest

or

bin/zenmusic -r ../res -c ../tst/test/cfg -p ../tst/session.rec  

How to record a test session :

gmake/make rectest

or

tst/test_run.sh 0

or

bin/zenmusic -r ../res -c ../tst/test/cfg -s ../tst/session.rec

How to debug session recording :

lldb -- bin/zenmusicdev -f 800x600 -r ../res -c ../tst/test/cfg -s ../tst/session1.rec

How to check for memory leaks :

A dev build will auto-check zc managed memory blocks for leaks, reports it in the last stdout line.

For non-zc managed memory use valgrind :

valgrind --leak-check=full --show-leak-kinds=all --suppressions=tst/valgrind.supp bin/zenmusicdev

TODO

## Protocol

If you are re-recording the main test session, follow this protocol.
If you add a new feature please add it to a proper place in the protocol.
If you see SCREENSHOT take a screenshot by pressing PRTINSCREEN button

SESSION 0 - clean start, invalid library path, cancel library

  start with no config file

   - start app with valid res_path and cfg_path, cfg_path shouldn't contain any config file
   - library popup should show up
   - press accept
   - enter invalid path, press accept
   - press reject button

  expectations :

   - in case of invalid path popup should show "Location doesn't exists" warning message
   - in case of reject button app should close
 
SESSION 1 - valid library path, changing library, organizing library, deleting files

  start with no config file ( cfg_path shouldn't contain any config file )

   - start app with valid res_path and cfg_path, cfg_path shouldn't contain any config file
   - library popup should show up
   - enter "../tst/test/lib1", press accept
   - click on main display, check activity logs
   - SCREENSHOT
   - press settings
   - click on library path
   - press reject
   - click on library path
   - enter "../tst/test/lib2", press accept
   - click on main display, check activity logs
   - SCREENSHOT
   - click on library path
   - enter "../tst/test/lib1", press accept
   - click on main display, check activity logs
   - SCREENSHOT
   - press settings
   - click on "organize library"
   - press reject
   - click on "organize library"
   - press accept
   - click on main display, check activity logs
   - SCREENSHOT
   - right click on Alphaville - Forever Young - in the mood
   - select delete song
   - accept
   - press close app

  expectations

   - library popup should disappear in case of valid library, library should be read, songs should be analyzed
   - last log message should be visible in main display
   - activity log should show read progress and read files
   - library path should be valid in settings popup
   - library should change
   - library should change back
   - library should be orgranized after accepting organize
   - song should be deleted

SESSION 2 - library browsing, filtering, selection

   - start app
   - SCREENSHOT
   - scroll with scrollwheel / touchpad
   - scroll over visualization rects/next to rects
   - scroll with scroll bar
   - resize column
   - replace column
   - select songs with CTRL and SHIFT pressed
   - select songs with right click - select song, select range
   - click on filters icon
   - SCREENSHOT
   - click on genres and artists
   - click on clear filter bar
   - filter for artist
   - filter for genre
   - filter for tag
   - press close app

   expectations

   - song list shows valid songs
   - songs get selected
   - songs get filtered properly
   - song list should be scrollable by scrollwheel / touchpad
   - song list should be scrollable by scroll bar
   - columns should be resizable/rearrangable
   - scroll song list next to bottom visualizer rectangles, over right and bottom scroller to see if they don't block events

SESSION 3 - metadata editing

   - start app
   - click on metadata editor icon
   - click outside
   - right click on 10cc - I'm not in love
   - click on date field, add 1974
   - click on comments field, add "best song"
   - click on "add new image", enter "../tst/vader.jpeg"
   - click on accept
   - click on accept
   - clock on main info, check activity logs
   - SCREENSHOT

   - right click on songlist, click on "select all"
   - right click, select metadata editor
   - enter "best songs" in tag field
   - click on accept, click on accept
   - open activity logs
   - SCREENSHOT
   - click on close app

   expectations

   - metadata writes are visible in activity window
   - metadata changes in the files

SESSION 4 - playback, controls, remote control

   - start app
   - press play
   - scroll over seek bar
   - drag seek bar
   - press pause
   - double click on adele sil
   - pause
   - SCREENSHOT
   - press next
   - press prev
   - press shuffle
   - press next
   - press mute
   - scroll over volume bar
   - drag volume bar
   - double click on royksopp
   - close app

   expectations

   - songs are playing as expected
   - volume changes as expected
   - royksopp video plays in album viewer
   - visualizer works
   - play/skip counters should work, last player/last skipped should work

SESSION 5 - misc, fullscreen, about popup

   - start app
   - open about popup
   - SCREENSHOT
   - toggle fullscreen 5 times

   expectations

   - about popup should look okay, buttons should work
   - texture map should re-fill itself without problem

Automatic post-test checks

 - config file should be present under config directory, should contain correct values
 - library structure should mirror what you did under the sessions
 - database file should contain correct values
 - metadata should reflex what was done in the sessions

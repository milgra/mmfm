# Zen Music User Guide

Welcome to Zen Music player, visualizer and organizer! I put a lot of effort in the making of this application and I hope that using this application fills your heart with warmth, pride and childlike joy!

## Table of contents

1. First start  
2. The Main Interface  
3. The toolbar  
4. The library browser  
5. The search/filter bar  
6. The visualizer overlays  
7. Controlling the application  
8. Controlling playback  
9. Handling the library browser  
10. The search/filter bar  
11. Supporting user interfaces  
12. The metadata/tag editor  
13. The settings viewer/editor  
14. The about popup  
15. The activity popup  
16. Remote control  
17. Creating song collections  
18. Command line arguments  
19. Customizing the user interface

## 1. First start

Open a terminal window or your favorite application launcher and type

```
zenmusic
```

The application will show you the library selector popup page.  
Click on the red button if you want to quit the application.  
If you want to continue, enter the path to the folder on your machine where you store your music/music videos and click on the green button.  
[note: in the near future a file browser popup will be added to this part]   
Zen Music will parse your library and create a database from your songs. During parsing the parsed songs will be shown in the library browser.  
 
## 2. The Main Interface

Zen Music's main user interface has four main parts :  

- The toolbar
- The library browser
- The search/filter bar
- The visualizer overlays

## 3. The toolbar

The toolbar contains the controller buttons and information displays.

Buttons from left to right :

1. close application button
2. the play/pause/seek multifunctional controller knob
3. previous song button
4. toggle shuffle button
5. next song button
6. settings button
7. about button
8. metadata editor button
9. the mute/unmute/volume multifunctional controller knob
10. the maximize button

Information displays from the left to right :

1. Current playtime of song
2. Song length
3. Remaining playtime of song

## 4. The library browser

The library browser contains all songs and important file/metadata information about your songs in your library.

## 5. The search/filter bar

The search/filter bar has three parts :

- the genre/artist selector button
- the search/filter input field
- the clean search/filter input field button

## 6. The visualizer overlays

The left and the right rectangle are the visualization overlays, the rectangle on the center is the cover art/music video viewer.

## 7. Controlling the application

**To exit the application**  
Click on the close application button

**To maximize/minimize the application**  
Click on the maximize button

## 8. Controlling playback

**To play a song**  
a. Double click on its item in the library browser  
b. Click on the center of the play/pause/seek knob to continue playing current song ( if paused )  
c. Press SPACE to continue playing current song ( if paused )  

**To pause the current song**  
a. Press the center of the play/pause/seek knob  
b. Press SPACE  

**To seek/fast forward/rewind the song**  
a. scroll over the play/pause/seek knob    
b. click on the outer ring of the play/pause/seek knob    
c. drag on the outer ring of the play/pause/seek knob  

A green overlay over the selected song item indicatesd that it is the active song item.  

**To play the previous song**  
Click on the "previous song" button  

**To play the next song**  
Click on the "next song" button  

**To toggle shuffle mode**  
Click on the "shuffle songs" button   
If shuffle is toggled the next/prev song buttons will play randomly selected songs.  

**To mute audio output**  
Press the center of mute/unmute/volume knob  

**To unmute audio output**  
Press the center of mute/unmute/volume knob  

**How to set volume**  
a. scroll over the mute/unmute/volume knob   
b. click on the outer ring of the mute/unmute/volume knob  
c. drag on the outer ring of the mute/unmute/volume knob  

## 9. Handling the library browser

**To scroll the song list**  
a. scroll over the list  
b. click on the invisible scrollbar area on the right edge of the song list ( vertical ) or bottom edge of the song list ( horizontal )  
c. drag on the invisible scrollbar area on the right edge of the song list ( vertical ) or bottom edge of the song list ( horizontal )  

**To resize the column**  
Drag on the right edge of the header cell you want to resize.  

**To rearrange the columns**  
Drag and drop the header cell onto an other cell

**To select multiple songs**  
a. press CTRL while clicking over an item  
b. do a right click on a song and click on select/deselect   

**To select ranges of songs**  
a. press SHIFT while clicking over an item  
b. do a right click on a song and click on select range  

**How to select all songs**  
right click on a song and click on select all

**How to initiate metadata editing for a song**  
right click on a song and click on edit song info

**How to delete a song**  
right click on a song and select delete song

## 10. The search/filter bar

**To browse among the genres and artist you have in your library**  
click on the genre/artist selector button and select a genre/artist. It will be added to the search/filter input field.

**To enter search term manually**  
click in the search/filter input field and enter an arbitrary text

**To clean serach/filter field**  
click on the clean search/filter field button

## 11. Supporting user interfaces

Zen Music's supporting user interface are the following :

- The metadata/tag editor
- The settings viewer/editor
- The about popup
- The activity popup

## 12. The metadata/tag editor

**To start metadata editing**  
a. right click on a song in the library browser and click on edit song info  
b. select multiple songs in the library browser and right click and click on edit song info   
c. select a song in the library browser and click on metadata editor button in the toolbar  

The table view contains all known information about the song, in the upper part shows the metadata/tags encoded in the media file itself, the lower, pink part shows the file data and miscellaneous info about the song.

The cover art viewer in the upper right corner shows the cover art if available.

**How to edit metadata**  
click on the metadata you want to edit, delete back with backspace, enter custom text with the keyboard and press ESCAPE or click outside of the field. then click on accept button in the lower right corner of the metadata editor.

**How to update cover art**  
Click on add new image button and enter absolute path of image, then accept or press ENTER. Then click on accept button in the lower right corner of the metadata editor.

## 13. The settings viewer/editor

**How to activate**  
click on the settings button in the main toolbar

**How to change library**  
click on the Library Path item. Enter a valid path in the textfield of the popup, then press accept button.

**How to enable automatic library organization**  
click on Organize Library item. click on accept button.

**How to enable remote control**  
click on Remote Control item

## 14. The about popup

**How to activate**  
click on the about popup button in the main toolbar.
click on any item to open the corresponding functionality in the browser.

## 15. The activity popup

**How to activate**  
click on the main info display in the toolbar

## 16. Remote control

It is possible to remote control Zen Music.
      
**How to enable the feature**  
In settings popup, click on Remote Control, click on accept.

**How to use the feature**  
Zen Music opens up an UDP port on 23723. ( The port is configurable in the config file )
Send 1 byte packets to this port, 0x00 to play/pause 0x01 to play previous song, 0x02 to play next song

For example, my i3 config for remote control looks like this :
```
bindsym $mod+F10 exec echo -n "0" | nc -4u -w0 localhost 23723	
bindsym $mod+F9 exec echo -n "1" | nc -4u -w0 localhost 23723
bindsym $mod+F11 exec echo -n "2" | nc -4u -w0 localhost 23723
```

In KDE/GNOME you can also bind keys to commands.

## 17. Creating song collections

Zen Music doesn't have playlists but you can do something similar. You can add tags to songs in the metadata editor, so if you want for example a playlist for monday, select all the songs you want to include at once or one-by-one and add "monday" to the tags field. Then if you search for "monday" in the search field it will show all songs with "monday" tag. Feel free to add more values to tags field using comma as separator.

## 18. Command line arguments

```
-c --config= [config file]	 	 use config file for session
-r --resources= [resources folder] 	 use resources dir for session
-s --record= [recorder file] 		 record session to file
-p --replay= [recorder file] 		 replay session from file
-f --frame= [widthxheight] 		 initial window dimension
```

## 19. Customizing the user interface

The user interface uses html for structure description and css for design description. The location of the two file is under settings. Feel free to modify them, but beware, the html/css parser is very strict, follow strictly the original files syntax to achieve success. Also some parts are generated from code and cannot be set by css, I will wire out those parts soon.
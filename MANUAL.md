# MultiMedia File Manager User Guide

Welcome to Multimedia File Manager! I put a lot of effort in the making of this application and I hope that using this application fills your heart with warmth, pride and childlike joy!

## Table of contents

1. First start
2. The main interface
3. The main toolbar
4. The file browser
5. The file info browser
6. The content toolbar
7. The content viewer
8. The file operations context menu
9. Controlling the application  
10. Handling the file browser
11. Command line arguments
12. Customizing the user interface

## 1. First start

Open a terminal window or your favorite application launcher and type

```
mmfm
```

The application will show you the contents of the current folder.
 
## 2. The Main Interface

MMFM's main user interface has five main parts :

- The main toolbar
- The file browser
- The file info browser
- The content toolbar
- The content viewer

## 3. The main toolbar

Elements from left to right :

1. close application button
2. about/help button
3. progress info field
4. the sidebar( file info/clipboard ) toggle button
5. the maximize button

## 4. The file browser

The file browser shows all files in the current directory. Use the mouse or the arrow keys to navigate through files. Single click or move over a file with the arrow keys to view it. Double click on the double dot or press enter on it to go up a directory.

## 5. The file info browser

The file info browser shows all information about the currently selected file. To make it visible click on the file info browser button on the toolbar next to the maximize button.

## 6. The content toolbar

Elements from left to right :

1. previous page button
2. next page button
3. play/pause button
4. progress bar
5. zoom in button
6. zoom out button

## 7. The content viewer

The content viewer shows the currently selected media. Press +/- buttons to zoom in/out, or use the scroll wheel of the mouse, or use pinch gestures on the touchpad to zoom in/out, or use the zoom in/out buttons in the content toolbar. Press space or the play/pause button to toggle auto play.

## 8. The file operations context menu ##

Elements from top to bottom :

1. Create folder button
2. Rename button
3. Delete button
4. Send to clipboard button
5. Paste using copy button
6. Paste using move button
7. Reset clipboard button

## 9. Controlling the application

**To exit the application**  
Click on the close application button

**To maximize/minimize the application**  
Click on the maximize button

## 10. Handling the file browser

**To scroll the song list**  
a. scroll over the list  
b. click on the invisible scrollbar area on the right edge of the song list ( vertical ) or bottom edge of the song list ( horizontal )  
c. drag on the invisible scrollbar area on the right edge of the song list ( vertical ) or bottom edge of the song list ( horizontal )  

**To resize the column**  
Drag on the right edge of the header cell you want to resize.  

**To rearrange the columns**  
Drag and drop the header cell onto an other cell

## 11. Command line arguments

```
-c --config= [config file]	 	 use config file for session
-r --resources= [resources folder] 	 use resources dir for session
-s --record= [recorder file] 		 record session to file
-p --replay= [recorder file] 		 replay session from file
-f --frame= [widthxheight] 		 initial window dimension
```

## 12. Customizing the user interface

The user interface uses html for structure description and css for design description. The location of the two file is under settings. Feel free to modify them, but beware, the html/css parser is very strict, follow strictly the original files syntax to achieve success. Also some parts are generated from code and cannot be set by css, I will wire out those parts soon.
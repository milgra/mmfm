# Multimedia File Manager User Guide

Welcome to Multimedia File Manager! I put a lot of effort in the making of this application and I hope that using this application fills your heart with warmth, pride and childlike joy!

## Table of contents

1. First start
2. The main interface
3. The toolbar
4. The file browser
5. The content viewer
6. The file info browser
7. Controlling the application  
8. Handling the file browser
9. Command line arguments
10. Customizing the user interface

## 1. First start

Open a terminal window or your favorite application launcher and type

```
mmfm
```

The application will show you the contents of the current folder.
 
## 2. The Main Interface

Zen Music's main user interface has four main parts :  

- The toolbar
- The file browser
- The content viewer
- The file info browser

## 3. The toolbar

The toolbar contains the controller buttons and information display.

Buttons from left to right :

1. close application button
2. progress info field
3. the file info browser button
4. the maximize button

## 4. The file browser

The file browser shows all files in the current directory. Use the mouse or the arrow keys to navigate through files. Double click on the double dot or press enter on it to go up a directory.

## 5. The content viewer

The content viewer shows the currently selected media. Press the control key and scroll with the mouse over the media to zoom in/out.

## 6. The file info browser

The file info browser shows all information about the currently selected file. To make it visible click on the file info browser button on the toolbar next to the maximize button.

## 7. Controlling the application

**To exit the application**  
Click on the close application button

**To maximize/minimize the application**  
Click on the maximize button

## 8. Handling the file browser

**To scroll the song list**  
a. scroll over the list  
b. click on the invisible scrollbar area on the right edge of the song list ( vertical ) or bottom edge of the song list ( horizontal )  
c. drag on the invisible scrollbar area on the right edge of the song list ( vertical ) or bottom edge of the song list ( horizontal )  

**To resize the column**  
Drag on the right edge of the header cell you want to resize.  

**To rearrange the columns**  
Drag and drop the header cell onto an other cell

## 9. Command line arguments

```
-c --config= [config file]	 	 use config file for session
-r --resources= [resources folder] 	 use resources dir for session
-s --record= [recorder file] 		 record session to file
-p --replay= [recorder file] 		 replay session from file
-f --frame= [widthxheight] 		 initial window dimension
```

## 10. Customizing the user interface

The user interface uses html for structure description and css for design description. The location of the two file is under settings. Feel free to modify them, but beware, the html/css parser is very strict, follow strictly the original files syntax to achieve success. Also some parts are generated from code and cannot be set by css, I will wire out those parts soon.
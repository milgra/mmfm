# Multimedia File Manager technical information
for contributors and developers

## 1. Overview ##

Multimedia File Manager is a pure C project written in [headerless C](https://github.com/milgra/headerlessc).
It uses the ffmpeg library for media decoding/encoding/transcoding, SDL2 library for window and audio handling and OpenGL context creation, mupdf for pdf rendering and freetype for text generation.
It uses a custom UI renderer called Zen UI.
It uses the Zen Core library for memory management, map/vector/bitmap container implementations, utf8 string and math functions.
For database and persistent storage it uses simple key-value text files.

## 2. Technology stacks ##

Graphics stack :

```
[OS][GPU] -> [SDL2] -> [ZEN_WM] -> [ZEN UI] -> ui.c -> ui controllers
```

Media stack :

```
[OS] -> [SDL2][ffmpeg] -> [coder.c]
     		       	  [viewer.c]
```

Database stack :

```
kvlist.c -> database.c
	    config.c
```

## 3. Project structure ##

```
doc - documentation
res - resources, html, css and images for the ui
src - code source files
 modules - zen modules
 mmfm - mmfm logic
svg - media source files
tst - recorded test sessions and test working directory
```
 
## 4. Multimedia File Manager Logic ##

```
config.c - configuration settings collector & writer
evrecroder.c - event recorder for test sessions
filemanager.c - file managing functions
fontconfig.c - extracts font path from os environment
kvlist.c - key-value list reader & writer
mmfm.c - entering point, main controller logic
pdf.c - pdf renderer
```

## 5. Program Flow ##

Entering point is in mmfm.c. It inits Zen WM module that inits SDL2 and openGL. Zen WM calls four functions in zenmusic.c :
- init - inits all zen music logic
- update - updates program state
- render - render UI
- destroy - cleans up memory

Internal state change happens in response to a UI event or timer event. All events are coming from Zen WM. In update they are sent to the UI and all views.
The views change their state based on the events and may call callbacks.

## 6. Development ##

Use your preferred IDE. It's advised that you hook clang-format to file save.

Please follow these guidelines :

- use clang format before commiting/after file save
- use zen_core functions and containers and memory handling
- create a new test for any new feature you add
- if you modify existing code be sure that a test covers your modification
- always run all tests before push
- always run valgrind and check for leaks before push

To create a dev build, type

```
CC=clang meson build --buildtype=debug -Db_sanitize=address -Db_lundef=false
ninja -C build

```
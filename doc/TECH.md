# Zen Music technical information
for contributors and developers

## 1. Overview ##

Zen Music is a pure C project written in [headerless C](https://github.com/milgra/headerlessc).
It uses the ffmpeg library for media decoding/encoding/transcoding, SDL2 library for window and audio handling and OpenGL context creation.
It uses a custom UI renderer called Zen UI, it is backed by OpenGL at the moment, Vulkan backend is on the roadmap.
It uses the Zen Core library for memory management, map/vector/bitmap container implementations, utf8 string and math functions.
For database and persistent storage it uses simple key-value text files.

## 2. Technology stacks ##

Graphics stack :

```
[OS][GPU] -> [SDL2] -> [ZEN_WM] -> [ZEN UI] -> ui.c -> ui controllers
```

Media stack :

```
[OS] -> [SDL2][ffmpeg] -> [ZEN MEDIA PLAYER]
     		       	  [ZEN MEDIA TRANSCODER]
```

Database stack :

```
kvlist.c -> database.c
	    config.c
```

## 3. Project structure ##

```
bin - build directory and built executable files
doc - documentation
res - resources, html, css and images for the ui
src - code source files
 modules - external modules
 zenmusic - zen music logic
  ui - zen music ui controllers
svg - media source files
tst - recorded test sessions and test working directory
```
 
## 4. Zen Music Logic ##

```
callbacks.c - callback collector & invocator
config.c - configuration settings collector & writer
database.c - song database handler & writer
evrecroder.c - event recorder for test sessions
files.c - file handling helper functions
kvlist.c - key-value list reader & writer
library.c - media library parser/modifier
remote.c - remote control handler
selection.c - selected item collector for library browser
visible.c - visible item handler ( after filtering & sort )
zenmusic.c - entering point, main controller logic
```

## 5. Program Flow ##

Entering point is in zenmusic.c. It inits Zen WM module that inits SDL2 and openGL. Zen WM calls four functions in zenmusic.c :
- init - inits all zen music logic
- update - updates program state
- render - render UI
- destroy - cleans up memory

Internal state change happens in response to a UI event or timer event. All events are coming from Zen WM. In update they are sent to the UI and all views.
The views change their state based on the events and may call callbacks.
View related logic is in the corresponding view controller. For example, metadata editing logic in in src/zenmusic/ui/ui_editor_popup.c .

## 5. Development ##

The following packages are needed for development :

```
sdl2 ffmpeg glew gmake clang-format
```

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
gmake dev
```
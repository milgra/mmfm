# MultiMedia File Manager

MultiMedia File Manager is a modern, lightning fast media viewer and manager for multimedia and document files. It can play and view everything ffmpeg and mupdf can.

MultiMedia File Manager was made for [SwayOS](https://swayos.github.io).

![alt text](screenshot.png)

## Features ##

- It aims to be the fastest & most responsive doc/media viewer out there
- Optimized both for keyboard and touchpad use
- MacOS-like scroll and pinch smoothness
- Instant preview for all known media formats and for pdf files
- Beautiful and smooth UX experience
- Written in super fast headerless C
- Super detailed file info - extracts all info available via stat and file commands and ffmpeg stream info
- Activity bar for transparent background processes
- Super lightweight, uses its own UI renderer (no QT/GTK )

Read the user guide for further information : [Open User Guide](MANUAL.md)

## Installation ##

Run these commands:

```
git clone https://github.com/milgra/mmfm.git
cd mmfm
meson build --buildtype=release
ninja -C build
sudo ninja -C build install
```

### From packages

[![Packaging status](https://repology.org/badge/tiny-repos/mmfm.svg)](https://repology.org/project/mmfm/versions)

## User Guide ##

Watch the user guide [video on youtube](https://youtube.com/)

Or read the [User Guide](MANUAL.md)

## Long descripton ##

MultiMedia File Manager is a modern, lightning fast media viewer and manager for multimedia and document files. It can play and view everything ffmpeg and mupdf can. It was created to make multimedia and document file organization a breeze.
If you move over a file with arrow keys or the select a file with the mouse it immediately shows its content/starts playing. If autoplay annoys you just switch autoplay off with space or with the play/pause button.
The file informaton also shows up immediately in the file info view.
You can freely resize any content inside the preview area with touchpad pinch gestures, with the scroll wheel, with plus/minus keys and the plus/minus buttons on the toolbar.
With a right click over the file list view or by pressing CTRL-V the file operation context menu pops up. You can delete and rename files from here, create a new folder and handle the clipboard from here.
The clipboard table is a crucial part of MultiMedia File Manager. You can open it by pressing the sidebar button or pressing CTRL+S. Here you can create a virtual list of files you want to work with. To send a file here select "Send file to clipboard" from the file operations context menu, or press CTRL+C over the file, or drag and drop the files here.
To reset the clipboard, select "Reset clipboard" from the file operations context menu.
To use these files in the file list view, select "Paste using copy" or "Paste using move" from the file ops menu or drag and drop files from the clipboard table on the file list table.

## Libraries used - Thanks for creating these! ##

- FFMPEG / media playing and parsing
- mupdf / pdf rendering
- SDL2 / audio handling
- freetype / text generation
- Neil Hanning's utf8.h / case-insensitive utf8 comparison

## License ##

MultiMedia File Manager is released under the GPLv3 (or later) license.
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

## Feedback ##

Please report issues and add feature requests here on github.

## Libraries used - Thanks for creating these! ##

- FFMPEG / media playing and parsing
- mupdf / pdf rendering
- SDL2 / audio handling
- freetype / text generation
- Neil Hanning's utf8.h / case-insensitive utf8 comparison

## License ##

MultiMedia File Manager is released under the GPLv3 (or later) license.
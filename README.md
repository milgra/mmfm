# Multimedia File Manager

Multimedia File Manager is a modern, lightning fast media viewer and manager for multimedia and document files. It can play and view everything ffmpeg and mupdf can.

Multimedie File Manager was made for [SwayOS](https://swayos.github.io).

## Features ##

- It aims to be the fastest & most responsive doc/media viewer out there
- Optimized for keyboard and touchpad use
- MacOS-like scroll and pinch smoothness
- Instant preview for all known media formats and for pdf files
- Beautiful and smooth UX experience
- Written in super fast headerless C
- Super detailed file info - extracts all info available via stat and file commands
- Activity bar and human-readable database for transparent operation
- Super lightweight, uses its own UI renderer (no QT/GTK )

Read the user guide for further information : [Open User Guide](doc/USER.md)

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

[Open User Guide](doc/USER.md)

## Feedback ##

Please report issues and add feature requests here on github.

## Libraries used - Thanks for creating these! ##

- FFMPEG / media parsing
- mupdf / pdf rendering
- SDL2 / window/graphics context handling
- freetype / text generation
- Neil Hanning's utf8.h / case-insensitive utf8 comparison

## Programs used - Thanks for creating these! ##

- Inkscape for the icons
- GNU Emacs for programming
- FreeBSD for development platform

## Contribute ##

Contributors are welcome!

## Tech Guide ##

[Tech Guide](doc/TECH.md)

## Roadmap ##

[Roadmap](doc/ROAD.md)

## License ##

MultiMedia File Manager is released under the GPLv3 (or later) license.
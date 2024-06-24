# MultiMedia File Manager

- drag and drop not working
- external file drag and drop
- delete accept + enter belep a folderbe ha az kovetkezik
- switchable texture smoothing for preview
- kurzor valtozzon at column move eseten
- play button active amikor mnme kene
- hold gesture to image scroller also
- first drag and drop doesn't work
- drag and drop from clipboard
- CTRL + drag from clipboard - move
- save autoplay state
- file list ordering head click
- store table states
- left right arrow scroll in scroll views
- get dimensions before image,pdf, video load, resize preview
- slow click rename
- test scaling
- copy/paste text from wayland clipboard
- text file load/seek, hexa view, fix meretu chunkokat olvasson
- linelist component, text fileoknak es hexa viewnak
- pause/play felvillano ikon over preview
- visualization level gombok, kep, oszcilloszkop, binaris
- search bar
- shortcutok config fileba vagy kulon fileba
- listen for folder change event
- HEIF/HEVC/mpts play error
- korusos videok nem jatszodnak le

- video on github page
- tech video, automated ui/full test, logban meg a leakek is lathatoak

- show raw html/css switch without any code
- legalso elem ne legyen felig takarva cursor down eseten
- input text scroll or wrap  
- dirty rect drawing for debugging
- time based animations
- unified css for all applications to make global styling easier

- Extracts all stream and metadata info from multimedia files, also shows the raw hexa/ASCII bytes of the file if needed
- Works without a window manager for super hackers
- Frequency and scope analyzer visualizers for audio conent

KineticUI is a UI renderer / UI component framework I created to bring MacOS level UI and UX to UNIX-like operating systems. It does kinetic scrolling, hold on two finger touch, edge bounce, zoom on pinch, glyph animations and much more. But one of it's most important feature is deterministic session recording and replaying to automate UI and functional testing. If you create screenshots during session recording with the built-in screenshot tool, those screenshots will be re-created during session replay and you can compare them for changes. If there is no random, time-bound or backgrounud thread-bound information in the screemshots then they should be identical on pixel-level. Check out MultiMedia File Manager ( www.github.com/milgra/mmfm ), Visual Music Player ( www.github.com/milgra/vmp ) , Sway Overview ( www.github.com/milgra/sov ) or Wayland Control Panel (www.github.com/milgra/wcp ) to see KineticUI in action!
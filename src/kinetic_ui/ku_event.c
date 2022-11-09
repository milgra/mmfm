#ifndef ku_event_h
#define ku_event_h

#include <stdint.h>
#include <time.h>

enum evtype
{
    KU_EVENT_EMPTY,
    KU_EVENT_FRAME,
    KU_EVENT_TIME,
    KU_EVENT_RESIZE,
    KU_EVENT_MMOVE,
    KU_EVENT_MDOWN,
    KU_EVENT_MUP,
    KU_EVENT_MMOVE_OUT,
    KU_EVENT_MDOWN_OUT,
    KU_EVENT_MUP_OUT,
    KU_EVENT_SCROLL,
    KU_EVENT_KDOWN,
    KU_EVENT_KUP,
    KU_EVENT_TEXT,
    KU_EVENT_WINDOW_SHOW,
    KU_EVENT_PINCH,
    KU_EVENT_STDIN,
    KU_EVENT_FOCUS,
    KU_EVENT_UNFOCUS,
};

typedef struct _ku_event_t
{
    enum evtype type;

    /* poiniter properties */

    int x; // mouse coord x
    int y; // mouse coord y

    int drag;   // mouse drag
    int button; // mouse button id

    /* keyboard modifiers */

    int ctrl_down;  // modifiers
    int shift_down; // modifiers

    /* scroll */

    float dx; // scroll x
    float dy; // scroll y

    /* touchpad */

    float ratio; // pinch ratio

    int w; // resize width
    int h; // resize height

    uint32_t        time;       // milliseconds since start
    struct timespec time_unix;  // unix timestamp
    float           time_frame; // elapsed time since last frame

    char     text[8];
    int      dclick;
    uint32_t keycode;
    int      repeat; // key event is coming from repeat

} ku_event_t;

#endif

#if __INCLUDE_LEVEL__ == 0

#endif

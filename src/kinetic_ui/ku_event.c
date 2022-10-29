#ifndef ku_event_h
#define ku_event_h

#include <stdint.h>

enum evtype
{
    KU_EVENT_EMPTY,
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
};

typedef struct _ku_event_t
{
    enum evtype type;

    int x;
    int y;
    int w;
    int h;

    float dx;
    float dy;

    uint32_t time;
    uint32_t dtime;

    char text[32];
    int  drag;

    int button;
    int dclick;
    int ctrl_down;
    int shift_down;

    int keycode;
} ku_event_t;

#endif

#if __INCLUDE_LEVEL__ == 0

#endif

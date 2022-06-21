#ifndef event_h
#define event_h

#include <SDL.h>
#include <stdint.h>

enum evtype
{
  EV_EMPTY,
  EV_TIME,
  EV_RESIZE,
  EV_MMOVE,
  EV_MDOWN,
  EV_MUP,
  EV_MMOVE_OUT,
  EV_MDOWN_OUT,
  EV_MUP_OUT,
  EV_SCROLL,
  EV_KDOWN,
  EV_KUP,
  EV_TEXT,
};

typedef struct _ev_t
{
  enum evtype type;
  int         x;
  int         y;
  float       dx;
  float       dy;
  int         w;
  int         h;
  char        text[32];
  int         drag;
  uint32_t    time;
  uint32_t    dtime;
  int         button;
  int         dclick;
  int         ctrl_down;
  int         shift_down;
#ifdef __linux__
  SDL_Keycode keycode;
#else
  SDL_KeyCode keycode;
#endif
} ev_t;

#endif

#if __INCLUDE_LEVEL__ == 0

#endif

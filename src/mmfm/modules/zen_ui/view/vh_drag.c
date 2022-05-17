#ifndef vh_drag_h
#define vh_drag_h

#include "view.c"
#include "wm_event.c"

void vh_drag_add(view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

void vh_drag_evt(view_t* view, ev_t ev)
{
  if (ev.type == EV_MMOVE && ev.drag)
  {
    r2_t frame = view->frame.local;
    frame.x    = ev.x;
    frame.y    = ev.y;
    view_set_frame(view, frame);
  }
}

void vh_drag_add(view_t* view)
{
  assert(view->handler == NULL && view->handler_data == NULL);

  view->handler_data = vh_drag_evt;
}

#endif

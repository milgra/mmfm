#ifndef vh_dragger_h
#define vh_dragger_h

#include "view.c"
#include "wm_event.c"

typedef struct _vh_dragger_t
{
    view_t* dragged_view;
} vh_dragger_t;

void vh_dragger_add(view_t* view);
void vh_dragger_drag(view_t* view, view_t* item);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

void vh_dragger_evt(view_t* view, ev_t ev)
{
    if (ev.type == EV_MMOVE && ev.drag)
    {
	r2_t frame = view->frame.local;
	frame.x    = ev.x - frame.w / 2;
	frame.y    = ev.y - frame.h / 2;
	view_set_frame(view, frame);
    }
}

void vh_dragger_del(void* p)
{
    // vh_dragger_t* vh = p;
}

void vh_dragger_desc(void* p, int level)
{
    printf("vh_drag");
}

void vh_dragger_add(view_t* view)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_dragger_t* vh = CAL(sizeof(vh_dragger_t), vh_dragger_del, vh_dragger_desc);

    view->needs_touch  = 1;
    view->handler_data = vh;
    view->handler      = vh_dragger_evt;
}

#endif

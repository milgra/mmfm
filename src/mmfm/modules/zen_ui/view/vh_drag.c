#ifndef vh_drag_h
#define vh_drag_h

#include "view.c"
#include "wm_event.c"

typedef struct _vh_drag_t
{
    view_t* dragged_view;
} vh_drag_t;

void vh_drag_attach(view_t* view);
void vh_drag_drag(view_t* view, view_t* item);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

void vh_drag_evt(view_t* view, ev_t ev)
{
    if (ev.type == EV_MMOVE && ev.drag)
    {
	vh_drag_t* vh = view->handler_data;

	if (vh->dragged_view)
	{
	    r2_t frame = vh->dragged_view->frame.local;
	    frame.x    = ev.x;
	    frame.y    = ev.y;
	    view_set_frame(vh->dragged_view, frame);
	}
    }
    if (ev.type == EV_MUP && ev.drag)
    {
	vh_drag_t* vh = view->handler_data;

	if (vh->dragged_view)
	{
	    view_remove_from_parent(vh->dragged_view);
	    REL(vh->dragged_view);
	    vh->dragged_view = NULL;
	}
    }
}

void vh_drag_del(void* p)
{
    // vh_drag_t* vh = p;
}

void vh_drag_desc(void* p, int level)
{
    printf("vh_drag");
}

void vh_drag_attach(view_t* view)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_drag_t* vh = CAL(sizeof(vh_drag_t), vh_drag_del, vh_drag_desc);

    view->needs_touch  = 1;
    view->handler_data = vh;
    view->handler      = vh_drag_evt;
}

void vh_drag_drag(view_t* view, view_t* item)
{
    vh_drag_t* vh = view->handler_data;

    if (vh->dragged_view)
    {
	view_remove_from_parent(vh->dragged_view);
	REL(vh->dragged_view);
	vh->dragged_view = NULL;
    }
    if (item)
    {
	vh->dragged_view = RET(item);
	view_add_subview(view, item);
    }
}

#endif

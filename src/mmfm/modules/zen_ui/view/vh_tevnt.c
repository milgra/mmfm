#ifndef vh_tevnt_h
#define vh_tevnt_h

#include "vh_tbody.c"
#include "view.c"

typedef struct _vh_tevnt_t
{
    view_t* tbody_view;
    void*   userdata;
} vh_tevnt_t;

void vh_tevnt_attach(
    view_t* view,
    view_t* tbody_view,
    void*   userdata);

#endif

#if __INCLUDE_LEVEL__ == 0

void vh_tevnt_evt(view_t* view, ev_t ev)
{
    vh_tevnt_t* vh = view->handler_data;

    if (ev.type == EV_TIME)
    {
    }
    else if (ev.type == EV_SCROLL)
    {
	vh_tbody_move(vh->tbody_view, ev.dx, ev.dy);
    }
    else if (ev.type == EV_RESIZE)
    {
	vh_tbody_move(vh->tbody_view, 0, 0);
    }
}

void vh_tevnt_del(void* p)
{
}

void vh_tevnt_desc(void* p, int level)
{
    printf("vh_tevnt");
}

void vh_tevnt_attach(
    view_t* view,
    view_t* tbody_view,
    void*   userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_tevnt_t* vh = CAL(sizeof(vh_tevnt_t), vh_tevnt_del, vh_tevnt_desc);
    vh->userdata   = userdata;
    vh->tbody_view = tbody_view;

    view->handler_data = vh;
    view->handler      = vh_tevnt_evt;
}

#endif

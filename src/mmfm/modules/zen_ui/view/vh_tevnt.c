#ifndef vh_tevnt_h
#define vh_tevnt_h

#include "vh_tbody.c"
#include "view.c"

typedef struct _vh_tevnt_t
{
    view_t* tbody_view;
    void*   userdata;
    int     is_absorbing;
    float   sx;
    float   sy;
} vh_tevnt_t;

void vh_tevnt_attach(
    view_t* view,
    view_t* tbody_view,
    void*   userdata);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

#define SWAY 50.0

void vh_tevnt_evt(view_t* view, ev_t ev)
{
    vh_tevnt_t* vh = view->handler_data;

    if (ev.type == EV_TIME)
    {
	vh_tbody_t* bvh = vh->tbody_view->handler_data;

	if (bvh->items && bvh->items->length > 0)
	{
	    if (vh->sy > 0.001 || vh->sy < -0.001 || vh->sx > 0.001 || vh->sx < -0.001)
	    {
		view_t* head = vec_head(bvh->items);
		view_t* tail = vec_tail(bvh->items);

		float top = head->frame.local.y;
		float bot = tail->frame.local.y + tail->frame.local.h;
		float hth = bot - top;
		float lft = head->frame.local.x;
		float rgt = head->frame.local.x + head->frame.local.w;

		vh->sx *= 0.8;
		vh->sy *= 0.8;

		vh_tbody_move(vh->tbody_view, vh->sx, vh->sy);

		if (hth > view->frame.local.h && top > 0.001) vh_tbody_move(vh->tbody_view, 0, -top / 5.0);
		if (hth > view->frame.local.h && bot < view->frame.local.h - 0.001) vh_tbody_move(vh->tbody_view, 0, (view->frame.local.h - bot) / 5.0);
		if (lft > 0.01) vh_tbody_move(vh->tbody_view, -lft / 5.0, 0.0);
		if (rgt < view->frame.local.w - 0.01) vh_tbody_move(vh->tbody_view, (view->frame.local.w - rgt) / 5.0, 0.0);
	    }
	}
    }
    else if (ev.type == EV_SCROLL)
    {
	vh->sx -= ev.dx;
	vh->sy += ev.dy;
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

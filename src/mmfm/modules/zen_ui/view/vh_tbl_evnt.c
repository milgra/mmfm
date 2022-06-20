#ifndef vh_tbl_evnt_h
#define vh_tbl_evnt_h

#include "vh_tbl_body.c"
#include "vh_tbl_head.c"
#include "vh_tbl_scrl.c"
#include "view.c"
#include <limits.h>

typedef struct _vh_tbl_evnt_t
{
    view_t* tbody_view;
    view_t* tscrl_view;
    view_t* thead_view;
    void*   userdata;
    int     scroll_drag;
    int     scroll_visible;
    view_t* selected_item;
    int     selected_index;
    float   sx;
    float   sy;
    void (*on_select)(view_t* view, view_t* rowview, int index, ev_t ev, void* userdata);
    void (*on_drag)(view_t* view, void* userdata);
} vh_tbl_evnt_t;

void vh_tbl_evnt_attach(
    view_t* view,
    view_t* tbody_view,
    view_t* tscrl_view,
    view_t* thead_view,
    void (*on_select)(view_t* view, view_t* rowview, int index, ev_t ev, void* userdata),
    void (*on_drag)(view_t* view, void* userdata),
    void* userdata);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

#define SCROLLBAR 20.0

void vh_tbl_evnt_evt(view_t* view, ev_t ev)
{
    vh_tbl_evnt_t* vh = view->handler_data;

    if (ev.type == EV_TIME)
    {
	vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

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

		float dx = vh->sx;
		float dy = vh->sy;

		if (hth > view->frame.local.h && top > 0.001) dy -= top / 5.0;
		if (hth > view->frame.local.h && bot < view->frame.local.h - 0.001) dy += (view->frame.local.h - bot) / 5.0;
		if (lft > 0.01) dx -= lft / 5.0;
		if (rgt < view->frame.local.w - 0.01) dx += (view->frame.local.w - rgt) / 5.0;

		vh_tbl_body_move(vh->tbody_view, dx, dy);
		if (vh->thead_view) vh_tbl_head_move(vh->thead_view, dx);

		if (vh->tscrl_view && vh->scroll_visible) vh_tbl_scrl_update(vh->tscrl_view);
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
	vh_tbl_body_move(vh->tbody_view, 0, 0);
    }
    else if (ev.type == EV_MMOVE)
    {
	// show scroll
	if (!vh->scroll_visible)
	{
	    vh->scroll_visible = 1;
	    vh_tbl_scrl_show(vh->tscrl_view);
	}
	if (vh->scroll_drag)
	{
	    if (ev.x > view->frame.global.x + view->frame.global.w - SCROLLBAR)
	    {
		vh_tbl_scrl_scroll_v(vh->tscrl_view, ev.y - view->frame.global.y);
	    }
	    if (ev.y > view->frame.global.y + view->frame.global.h - SCROLLBAR)
	    {
		vh_tbl_scrl_scroll_h(vh->tscrl_view, ev.x - view->frame.global.x);
	    }
	}
	if (vh->selected_item && ev.drag)
	{
	    if (vh->on_drag) (*vh->on_drag)(view, vh->userdata);

	    vh->selected_item = NULL;
	}
    }
    else if (ev.type == EV_MMOVE_OUT)
    {
	// hide scroll
	if (vh->scroll_visible)
	{
	    vh->scroll_visible = 0;
	    vh_tbl_scrl_hide(vh->tscrl_view);
	}
    }
    else if (ev.type == EV_MDOWN)
    {
	if (ev.x > view->frame.global.x + view->frame.global.w - SCROLLBAR)
	{
	    vh->scroll_drag = 1;
	    vh_tbl_scrl_scroll_v(vh->tscrl_view, ev.y - view->frame.global.y);
	}
	if (ev.y > view->frame.global.y + view->frame.global.h - SCROLLBAR)
	{
	    vh->scroll_drag = 1;
	    vh_tbl_scrl_scroll_h(vh->tscrl_view, ev.x - view->frame.global.x);
	}
	if (ev.x < view->frame.global.x + view->frame.global.w - SCROLLBAR &&
	    ev.y < view->frame.global.y + view->frame.global.h - SCROLLBAR)
	{
	    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

	    for (int index = 0; index < bvh->items->length; index++)
	    {
		view_t* item = bvh->items->data[index];
		if (ev.x > item->frame.global.x &&
		    ev.x < item->frame.global.x + item->frame.global.w &&
		    ev.y > item->frame.global.y &&
		    ev.y < item->frame.global.y + item->frame.global.h)
		{
		    vh->selected_item  = item;
		    vh->selected_index = bvh->head_index + index;

		    if (vh->on_select) (*vh->on_select)(view, item, bvh->head_index + index, ev, vh->userdata);
		    break;
		}
	    }
	}
    }
    else if (ev.type == EV_MUP)
    {
	vh->scroll_drag = 0;
    }
}

void vh_tbl_evnt_del(void* p)
{
}

void vh_tbl_evnt_desc(void* p, int level)
{
    printf("vh_tbl_evnt");
}

void vh_tbl_evnt_attach(
    view_t* view,
    view_t* tbody_view,
    view_t* tscrl_view,
    view_t* thead_view,
    void (*on_select)(view_t* view, view_t* rowview, int index, ev_t ev, void* userdata),
    void (*on_drag)(view_t* view, void* userdata),
    void* userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_tbl_evnt_t* vh = CAL(sizeof(vh_tbl_evnt_t), vh_tbl_evnt_del, vh_tbl_evnt_desc);
    vh->on_select     = on_select;
    vh->on_drag       = on_drag;
    vh->userdata      = userdata;
    vh->tbody_view    = tbody_view;
    vh->tscrl_view    = tscrl_view;
    vh->thead_view    = thead_view;

    view->handler_data = vh;
    view->handler      = vh_tbl_evnt_evt;
}

#endif

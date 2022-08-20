/* content view event */

#ifndef vh_cv_evnt_h
#define vh_cv_evnt_h

#include "vh_cv_body.c"
#include "vh_cv_scrl.c"
#include "view.c"

typedef struct _vh_cv_evnt_t
{
    view_t* tbody_view;
    view_t* tscrl_view;
    void*   userdata;
    int     scroll_drag;
    int     scroll_visible;
    float   sx;
    float   sy;
    float   mx;
    float   my;
    float   z;
} vh_cv_evnt_t;

void vh_cv_evnt_attach(
    view_t* view,
    view_t* tbody_view,
    view_t* tscrl_view,
    void*   userdata);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

#define SCROLLBAR 20.0

void vh_cv_evnt_evt(view_t* view, ev_t ev)
{
    vh_cv_evnt_t* vh = view->handler_data;

    if (ev.type == EV_TIME)
    {
	vh_cv_body_t* bvh = vh->tbody_view->handler_data;

	vh->sx *= 0.8;
	vh->sy *= 0.8;

	vh_cv_body_move(vh->tbody_view, vh->sx, vh->sy);

	if (vh->z > 0.001 || vh->z < -0.001)
	{
	    vh->z *= 0.8;
	    vh_cv_body_zoom(vh->tbody_view, vh->z, vh->mx, vh->my);
	}

	if (vh->tscrl_view && vh->scroll_visible) vh_cv_scrl_update(vh->tscrl_view);

	vh_cv_scrl_t* svh = vh->tscrl_view->handler_data;

	if (svh->state > 0) vh_cv_scrl_update(vh->tscrl_view);
    }
    else if (ev.type == EV_SCROLL)
    {
	if (!ev.ctrl_down)
	{
	    vh->sx -= ev.dx;
	    vh->sy += ev.dy;
	}
	else
	{
	    vh->z += ev.dy;
	}
    }
    else if (ev.type == EV_RESIZE)
    {
    }
    else if (ev.type == EV_MMOVE)
    {
	// show scroll
	if (!vh->scroll_visible)
	{
	    vh->scroll_visible = 1;
	    vh_cv_scrl_show(vh->tscrl_view);
	}
	if (vh->scroll_drag)
	{
	    if (ev.x > view->frame.global.x + view->frame.global.w - SCROLLBAR)
	    {
		vh_cv_scrl_scroll_v(vh->tscrl_view, ev.y - view->frame.global.y);
	    }
	    if (ev.y > view->frame.global.y + view->frame.global.h - SCROLLBAR)
	    {
		vh_cv_scrl_scroll_h(vh->tscrl_view, ev.x - view->frame.global.x);
	    }
	}

	vh->mx = ev.x;
	vh->my = ev.y;
    }
    else if (ev.type == EV_MMOVE_OUT)
    {
	// hide scroll
	if (vh->scroll_visible)
	{
	    vh->scroll_visible = 0;
	    vh_cv_scrl_hide(vh->tscrl_view);
	}
    }
    else if (ev.type == EV_MDOWN)
    {
	if (ev.x > view->frame.global.x + view->frame.global.w - SCROLLBAR)
	{
	    vh->scroll_drag = 1;
	    vh_cv_scrl_scroll_v(vh->tscrl_view, ev.y - view->frame.global.y);
	}
	if (ev.y > view->frame.global.y + view->frame.global.h - SCROLLBAR)
	{
	    vh->scroll_drag = 1;
	    vh_cv_scrl_scroll_h(vh->tscrl_view, ev.x - view->frame.global.x);
	}
    }
    else if (ev.type == EV_MUP)
    {
	vh->scroll_drag = 0;
    }
}

void vh_cv_evnt_del(void* p)
{
}

void vh_cv_evnt_desc(void* p, int level)
{
    printf("vh_cv_evnt");
}

void vh_cv_evnt_attach(
    view_t* view,
    view_t* tbody_view,
    view_t* tscrl_view,
    void*   userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_cv_evnt_t* vh = CAL(sizeof(vh_cv_evnt_t), vh_cv_evnt_del, vh_cv_evnt_desc);
    vh->userdata     = userdata;
    vh->tbody_view   = tbody_view;
    vh->tscrl_view   = tscrl_view;

    view->handler_data = vh;
    view->handler      = vh_cv_evnt_evt;
}

#endif

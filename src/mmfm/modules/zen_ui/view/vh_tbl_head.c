#ifndef vh_tbl_head_h
#define vh_tbl_head_h

#include "view.c"
#include "zc_log.c"
#include "zc_vector.c"

typedef struct _vh_tbl_head_t
{
    view_t* (*head_create)(view_t* view, void* userdata);
    void (*head_resize)(view_t* hview, int index, int size);
    void (*head_reorder)(view_t* hview, int ind1, int ind2);
    void*   userdata;
    view_t* head;
    int     active;
    int     resize;
    int     touchx;
} vh_tbl_head_t;

void vh_tbl_head_attach(
    view_t* view,
    view_t* (*head_create)(view_t* hview, void* userdata),
    void (*head_resize)(view_t* hview, int index, int size),
    void (*head_reorder)(view_t* hview, int ind1, int ind2),
    void* userdata);

void vh_tbl_head_move(view_t* hview, float dx);

#endif

#if __INCLUDE_LEVEL__ == 0

#define EDGE_DRAG_SIZE 10

void vh_tbl_head_evt(view_t* view, ev_t ev)
{
    vh_tbl_head_t* vh = view->handler_data;

    if (vh->head)
    {
	if (ev.type == EV_MDOWN)
	{
	    // look for
	    for (int index = 0; index < vh->head->views->length; index++)
	    {
		view_t* sv  = vh->head->views->data[index];
		r2_t    svf = sv->frame.global;
		// inside
		if (ev.x > svf.x + EDGE_DRAG_SIZE &&
		    ev.x < svf.x + svf.w - EDGE_DRAG_SIZE)
		{
		    vh->active = index;
		    vh->touchx = ev.x - svf.x;
		    break;
		}
		// block start
		if ((ev.x < svf.x + EDGE_DRAG_SIZE) &&
		    ev.x >= svf.x)
		{
		    vh->resize = 1;
		    vh->active = index - 1;
		    break;
		}
		// block end
		if ((ev.x > svf.x + svf.w - EDGE_DRAG_SIZE) &&
		    ev.x < svf.x + svf.w)
		{
		    vh->resize = 1;
		    vh->active = index;
		    break;
		}
	    }
	}
	else if (ev.type == EV_MUP || ev.type == EV_MUP_OUT)
	{
	    vh->active = -1;
	    vh->resize = 0;
	}
	else if (ev.type == EV_MMOVE)
	{
	    if (vh->active > -1)
	    {
		if (vh->active < vh->head->views->length)
		{
		    view_t* sv   = vh->head->views->data[vh->active];
		    r2_t    svfg = sv->frame.global;
		    r2_t    svfl = sv->frame.local;
		    if (vh->resize)
		    {
			svfl.w = ev.x - svfg.x;
			view_set_frame(sv, svfl);

			// resize cells
			int pos = 0;
			for (int index = 0; index < vh->head->views->length; index++)
			{
			    view_t* sv = vh->head->views->data[index];
			    svfl       = sv->frame.local;
			    svfl.x     = pos;
			    pos += svfl.w;
			    view_set_frame(sv, svfl);
			}
		    }
		    else
		    {
			svfl.x = ev.x - view->frame.global.x - vh->touchx;
			view_set_frame(sv, svfl);
		    }
		}
	    }
	}
    }
}

void vh_tbl_head_move(
    view_t* view,
    float   dx)
{
    vh_tbl_head_t* vh = view->handler_data;

    r2_t frame = vh->head->frame.local;

    frame.x += dx;

    view_set_frame(vh->head, frame);
}

void vh_tbl_head_del(void* p)
{
}

void vh_tbl_head_desc(void* p, int level)
{
    printf("vh_tbl_head");
}

void vh_tbl_head_attach(
    view_t* view,
    view_t* (*head_create)(view_t* hview, void* userdata),
    void (*head_resize)(view_t* hview, int index, int size),
    void (*head_reorder)(view_t* hview, int ind1, int ind2),
    void* userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_tbl_head_t* vh = CAL(sizeof(vh_tbl_head_t), vh_tbl_head_del, vh_tbl_head_desc);
    vh->userdata      = userdata;
    vh->head_create   = head_create;
    vh->head          = (*head_create)(view, userdata);
    vh->active        = -1;

    view->handler_data = vh;
    view->handler      = vh_tbl_head_evt;
    view->needs_touch  = 1;

    view_add_subview(view, vh->head);
}

#endif

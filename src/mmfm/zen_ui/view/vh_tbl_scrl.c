#ifndef vh_tbl_scrl_h
#define vh_tbl_scrl_h

#include "vh_tbl_body.c"
#include "vh_tbl_head.c"
#include "view.c"

typedef struct _vh_tbl_scrl_t
{
    view_t*  tbody_view;
    view_t*  thead_view;
    view_t*  vert_v;
    view_t*  hori_v;
    uint32_t item_cnt;
    void*    userdata;
} vh_tbl_scrl_t;

void vh_tbl_scrl_attach(
    view_t* view,
    view_t* tbody_view,
    view_t* thead_view,
    void*   userdata);

void vh_tbl_scrl_update(view_t* view);
void vh_tbl_scrl_show(view_t* view);
void vh_tbl_scrl_hide(view_t* view);
void vh_tbl_scrl_set_item_count(view_t* view, uint32_t count);
void vh_tbl_scrl_scroll_v(view_t* view, int y);
void vh_tbl_scrl_scroll_h(view_t* view, int x);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

void vh_tbl_scrl_del(void* p)
{
    vh_tbl_scrl_t* vh = p;
    REL(vh->vert_v);
    REL(vh->hori_v);
}

void vh_tbl_scrl_desc(void* p, int level)
{
    printf("vh_tbl_scrl");
}

void vh_tbl_scrl_attach(
    view_t* view,
    view_t* tbody_view,
    view_t* thead_view,
    void*   userdata)
{
    vh_tbl_scrl_t* vh = CAL(sizeof(vh_tbl_scrl_t), vh_tbl_scrl_del, vh_tbl_scrl_desc);
    vh->userdata      = userdata;
    vh->tbody_view    = tbody_view;
    vh->thead_view    = thead_view;

    assert(view->views->length > 1);

    vh->vert_v = RET(view->views->data[0]);
    vh->hori_v = RET(view->views->data[1]);

    view->handler_data = vh;
}

void vh_tbl_scrl_set_item_count(view_t* view, uint32_t count)
{
    vh_tbl_scrl_t* vh = view->handler_data;

    vh->item_cnt = count;
}

void vh_tbl_scrl_update(view_t* view)
{
    vh_tbl_scrl_t* vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

    if (bvh->items->length > 0 && vh->item_cnt > 0)
    {
	view_t* head = bvh->items->data[0];

	int vert_pos = bvh->top_index;
	int vert_vis = bvh->bot_index - bvh->top_index;
	int vert_max = vh->item_cnt;

	if (vert_max > 1.0)
	{
	    float pratio = (float) vert_pos / (float) vert_max;
	    float sratio = (float) vert_vis / (float) vert_max;

	    r2_t frame = vh->vert_v->frame.local;
	    frame.h += (view->frame.local.h * sratio - frame.h) / 5.0;
	    frame.y += (view->frame.local.h * pratio - frame.y) / 5.0;

	    view_set_frame(vh->vert_v, frame);
	}

	float hori_pos = -head->frame.local.x;
	float hori_vis = view->frame.local.w;
	float hori_max = head->frame.local.w;

	if (hori_max > 1.0)
	{
	    float pratio = (float) hori_pos / (float) hori_max;
	    float sratio = (float) hori_vis / (float) hori_max;

	    r2_t frame = vh->hori_v->frame.local;
	    frame.w += (view->frame.local.w * sratio - frame.w) / 5.0;
	    frame.x += (view->frame.local.w * pratio - frame.x) / 5.0;

	    view_set_frame(vh->hori_v, frame);
	}
    }
}

void vh_tbl_scrl_show(view_t* view)
{
    vh_tbl_scrl_t* vh = view->handler_data;
    if (vh->vert_v->parent == NULL) view_add_subview(view, vh->vert_v);
    if (vh->hori_v->parent == NULL) view_add_subview(view, vh->hori_v);
}

void vh_tbl_scrl_hide(view_t* view)
{
    vh_tbl_scrl_t* vh = view->handler_data;
    view_remove_subview(view, vh->vert_v);
    view_remove_subview(view, vh->hori_v);
}

void vh_tbl_scrl_scroll_v(view_t* view, int y)
{
    vh_tbl_scrl_t* vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

    if (bvh->items->length > 0 && vh->item_cnt > 0)
    {
	int vert_pos = bvh->top_index;
	int vert_vis = bvh->bot_index - bvh->top_index;
	int vert_max = vh->item_cnt;

	if (vert_max > 1)
	{
	    float sratio = (float) vert_vis / (float) vert_max;
	    float height = (view->frame.local.h - view->frame.local.h * sratio);
	    float pratio = (float) y / height;

	    if (pratio < 0.0) pratio = 0.0;
	    if (pratio > 1.0) pratio = 1.0;
	    int topindex = pratio * (vert_max - vert_vis);

	    vh_tbl_body_vjump(vh->tbody_view, topindex);

	    r2_t frame = vh->vert_v->frame.local;
	    frame.h    = view->frame.local.h * sratio;
	    frame.y    = y;

	    view_set_frame(vh->vert_v, frame);
	}
    }
}

void vh_tbl_scrl_scroll_h(view_t* view, int x)
{
    vh_tbl_scrl_t* vh  = view->handler_data;
    vh_tbl_body_t* bvh = vh->tbody_view->handler_data;

    if (bvh->items->length > 0 && vh->item_cnt > 0)
    {
	view_t* head = bvh->items->data[0];

	float hori_pos = -head->frame.local.x;
	float hori_vis = view->frame.local.w;
	float hori_max = head->frame.local.w;

	if (hori_max > 1.0)
	{
	    float sratio = (float) hori_vis / (float) hori_max;
	    float width  = (view->frame.local.w - view->frame.local.w * sratio);
	    float pratio = (float) x / width;

	    if (pratio < 0.0) pratio = 0.0;
	    if (pratio > 1.0) pratio = 1.0;
	    float dx = pratio * (hori_max - hori_vis);

	    vh_tbl_body_hjump(vh->tbody_view, -dx);
	    vh_tbl_head_jump(vh->thead_view, -dx);

	    r2_t frame = vh->hori_v->frame.local;
	    frame.w    = view->frame.local.w * sratio;
	    frame.x    = x;

	    view_set_frame(vh->hori_v, frame);
	}
    }
}

#endif

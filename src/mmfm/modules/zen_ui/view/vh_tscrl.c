#ifndef vh_tscrl_h
#define vh_tscrl_h

#include "vh_tbody.c"
#include "view.c"

typedef struct _vh_tscrl_t
{
    view_t*  tbody_view;
    view_t*  vert_v;
    view_t*  hori_v;
    uint32_t item_cnt;
    void*    userdata;
} vh_tscrl_t;

void vh_tscrl_attach(
    view_t* view,
    view_t* tbody_view,
    void*   userdata);

void vh_tscrl_update(view_t* view);
void vh_tscrl_set_item_count(view_t* view, uint32_t count);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

void vh_tscrl_del(void* p)
{
}

void vh_tscrl_desc(void* p, int level)
{
    printf("vh_tscrl");
}

void vh_tscrl_attach(
    view_t* view,
    view_t* tbody_view,
    void*   userdata)
{
    vh_tscrl_t* vh = CAL(sizeof(vh_tscrl_t), vh_tscrl_del, vh_tscrl_desc);
    vh->userdata   = userdata;
    vh->tbody_view = tbody_view;

    assert(view->views->length > 1);

    vh->vert_v = view->views->data[0];
    vh->hori_v = view->views->data[1];

    view->handler_data = vh;
}

void vh_tscrl_set_item_count(view_t* view, uint32_t count)
{
    vh_tscrl_t* vh = view->handler_data;

    vh->item_cnt = count;
}

void vh_tscrl_update(view_t* view)
{
    vh_tscrl_t* vh  = view->handler_data;
    vh_tbody_t* bvh = vh->tbody_view->handler_data;

    if (bvh->items->length > 0 && vh->item_cnt > 0)
    {
	int vert_pos = bvh->top_index;
	int vert_vis = bvh->bot_index - bvh->top_index;
	int vert_max = vh->item_cnt;

	float pratio = (float) vert_pos / (float) vert_max;
	float sratio = (float) vert_vis / (float) vert_max;

	r2_t frame = vh->vert_v->frame.local;
	frame.h += (view->frame.local.h * sratio - frame.h) / 5.0;
	frame.y += (view->frame.local.h * pratio - frame.y) / 5.0;

	view_set_frame(vh->vert_v, frame);

	view_t* head = bvh->items->data[0];

	float hori_pos = -head->frame.local.x;
	float hori_vis = view->frame.local.w;
	float hori_max = head->frame.local.w;

	pratio = (float) hori_pos / (float) hori_max;
	sratio = (float) hori_vis / (float) hori_max;

	frame = vh->hori_v->frame.local;
	frame.w += (view->frame.local.w * sratio - frame.w) / 5.0;
	frame.x += (view->frame.local.w * pratio - frame.x) / 5.0;

	view_set_frame(vh->hori_v, frame);
    }
}

#endif

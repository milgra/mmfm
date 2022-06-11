#ifndef vh_tbl_head_h
#define vh_tbl_head_h

#include "view.c"
#include "zc_vector.c"

typedef struct _vh_tbl_head_t
{
    view_t* (*head_create)(view_t* view, void* userdata);
    void*   userdata;
    view_t* head;
} vh_tbl_head_t;

void vh_tbl_head_attach(
    view_t* view,
    view_t* (*head_create)(view_t* hview, void* userdata),
    void* userdata);

void vh_tbl_head_move(view_t* hview, float dx);

#endif

#if __INCLUDE_LEVEL__ == 0

void vh_tbl_head_evt(view_t* view, ev_t ev)
{
    vh_tbl_head_t* vh = view->handler_data;

    if (ev.type == EV_MMOVE)
    {
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
    void* userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_tbl_head_t* vh = CAL(sizeof(vh_tbl_head_t), vh_tbl_head_del, vh_tbl_head_desc);
    vh->userdata      = userdata;
    vh->head_create   = head_create;
    vh->head          = (*head_create)(view, userdata);

    view->handler_data = vh;
    view->handler      = vh_tbl_head_evt;

    view_add_subview(view, vh->head);
}

#endif

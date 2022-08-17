/* content view body */

#ifndef vh_cv_body_h
#define vh_cv_body_h

#include "view.c"

typedef struct _vh_cv_body_t
{
    void* userdata;

    view_t* content;
    float   xpos;
    float   ypos;

} vh_cv_body_t;

void vh_cv_body_attach(
    view_t* view,
    void*   userdata);

void vh_cv_body_move(
    view_t* view,
    float   dx,
    float   dy);

void vh_cv_body_reset(
    view_t* view);

void vh_cv_body_hjump(
    view_t* view,
    float   dx);

void vh_cv_body_vjump(
    view_t* view,
    int     topindex);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

void vh_cv_body_del(void* p)
{
    vh_cv_body_t* vh = p;
}

void vh_cv_body_desc(void* p, int level)
{
    printf("vh_cv_body");
}

void vh_cv_body_attach(
    view_t* view,
    void*   userdata)
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_cv_body_t* vh = CAL(sizeof(vh_cv_body_t), vh_cv_body_del, vh_cv_body_desc);
    vh->userdata     = userdata;
    vh->content      = view->views->data[0];

    view->handler_data = vh;
}

void vh_cv_body_move(
    view_t* view,
    float   dx,
    float   dy)
{
    vh_cv_body_t* vh = view->handler_data;

    r2_t frame = vh->content->frame.local;
    frame.x += dx;
    frame.y += dy;
    view_set_frame(vh->content, frame);
}

void vh_cv_body_reset(
    view_t* view)
{
    vh_cv_body_t* vh = view->handler_data;
}

void vh_cv_body_hjump(
    view_t* view,
    float   x)
{
    vh_cv_body_t* vh = view->handler_data;
}

void vh_cv_body_vjump(
    view_t* view,
    int     topindex)
{
    vh_cv_body_t* vh = view->handler_data;
}

#endif

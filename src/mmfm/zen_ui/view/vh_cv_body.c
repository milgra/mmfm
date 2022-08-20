/* content view body */

#ifndef vh_cv_body_h
#define vh_cv_body_h

#include "view.c"

typedef struct _vh_cv_body_t
{
    void* userdata;

    view_t* content;
    float   sx;
    float   sy;
    float   px;
    float   py;
    float   scale;
} vh_cv_body_t;

void vh_cv_body_attach(
    view_t* view,
    void*   userdata);

void vh_cv_body_set_content_size(
    view_t* view,
    int     sx,
    int     sy);

void vh_cv_body_move(
    view_t* view,
    float   dx,
    float   dy);

void vh_cv_body_zoom(
    view_t* view,
    float   s,
    int     x,
    int     y);

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

#include "tg_scaledimg.c"
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
    vh->scale        = 1.0;

    view->handler_data = vh;
}

void vh_cv_body_set_content_size(
    view_t* view,
    int     sx,
    int     sy)
{
    vh_cv_body_t* vh = view->handler_data;
    vh->sx           = sx;
    vh->sy           = sy;

    r2_t frame = vh->content->frame.local;
    frame.w    = sx;
    frame.h    = sy;

    tg_scaledimg_set_content_size(vh->content, frame.w, frame.h);
    view_set_frame(vh->content, frame);
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

void vh_cv_body_zoom(
    view_t* view,
    float   s,
    int     x,
    int     y)
{
    vh_cv_body_t* vh = view->handler_data;

    r2_t gf = vh->content->frame.global;
    r2_t lf = vh->content->frame.local;

    /* partial width and height from mouse position */

    float pw = (float) x - gf.x;
    float ph = (float) y - gf.y;

    /* ratios */

    float rw = pw / gf.w;
    float rh = ph / gf.h;

    /* new dimensions */

    vh->scale += s / 100.0;

    float nw = vh->sx * vh->scale;
    float nh = vh->sy * vh->scale;

    lf.x = (float) x - rw * nw - view->frame.global.x;
    lf.y = (float) y - rh * nh - view->frame.global.y;
    lf.w = nw;
    lf.h = nh;

    view_set_frame(vh->content, lf);
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

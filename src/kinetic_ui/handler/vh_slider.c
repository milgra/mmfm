#ifndef vh_slider_h
#define vh_slider_h

#include "ku_view.c"

typedef struct _vh_slider_t vh_slider_t;

typedef struct _vh_slider_event_t
{
    vh_slider_t* vh;
    ku_view_t*   view;
    float        ratio;
} vh_slider_event_t;

struct _vh_slider_t
{
    float ratio;
    void (*on_event)(vh_slider_event_t);
};

void  vh_slider_add(ku_view_t* view, void (*on_event)(vh_slider_event_t));
void  vh_slider_set(ku_view_t* view, float ratio);
float vh_slider_get_ratio(ku_view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <stdio.h>

void vh_slider_evt(ku_view_t* view, ku_event_t ev)
{
    if (ev.type == KU_EVENT_MDOWN || (ev.type == KU_EVENT_MMOVE && ev.drag))
    {
	vh_slider_t* vh = view->handler_data;

	float dx  = ev.x - view->frame.global.x;
	vh->ratio = dx / view->frame.global.w;

	ku_view_t* bar   = view->views->data[0];
	ku_rect_t  frame = bar->frame.local;
	frame.w          = dx;
	ku_view_set_frame(bar, frame);

	vh_slider_event_t event = {.view = view, .ratio = vh->ratio, .vh = vh};
	if (vh->on_event) (*vh->on_event)(event);
    }
    else if (ev.type == KU_EVENT_SCROLL)
    {
	vh_slider_t* vh = view->handler_data;

	float ratio = vh->ratio - ev.dx / 50.0;

	if (ratio < 0) ratio = 0;
	if (ratio > 1) ratio = 1;

	vh->ratio        = ratio;
	ku_view_t* bar   = view->views->data[0];
	ku_rect_t  frame = bar->frame.local;
	frame.w          = view->frame.global.w * vh->ratio;
	ku_view_set_frame(bar, frame);

	vh_slider_event_t event = {.view = view, .ratio = vh->ratio, .vh = vh};
	if (vh->on_event) (*vh->on_event)(event);
    }
}

void vh_slider_set(ku_view_t* view, float ratio)
{
    vh_slider_t* vh  = view->handler_data;
    vh->ratio        = ratio;
    ku_view_t* bar   = view->views->data[0];
    ku_rect_t  frame = bar->frame.local;
    frame.w          = view->frame.global.w * vh->ratio;
    ku_view_set_frame(bar, frame);
}

float vh_slider_get_ratio(ku_view_t* view)
{
    vh_slider_t* vh = view->handler_data;
    return vh->ratio;
}

void vh_slider_desc(void* p, int level)
{
    printf("vh_slider");
}

void vh_slider_del(void* p)
{
}

void vh_slider_add(ku_view_t* view, void (*on_event)(vh_slider_event_t))
{
    assert(view->handler == NULL && view->handler_data == NULL);

    vh_slider_t* vh = CAL(sizeof(vh_slider_t), vh_slider_del, vh_slider_desc);
    vh->on_event    = on_event;

    view->handler_data = vh;
    view->handler      = vh_slider_evt;
}

#endif

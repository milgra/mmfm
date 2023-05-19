#ifndef vh_key_h
#define vh_key_h

#include "ku_view.c"

typedef struct _vh_key_t vh_key_t;

typedef struct _vh_key_event_t
{
    ku_event_t ev;
    vh_key_t*  vh;
    ku_view_t* view;
} vh_key_event_t;

struct _vh_key_t
{
    int (*on_event)(vh_key_event_t event);
};

void vh_key_add(ku_view_t* view, int (*on_event)(vh_key_event_t));

#endif

#if __INCLUDE_LEVEL__ == 0

int vh_key_evt(ku_view_t* view, ku_event_t ev)
{
    int cancel = 0;
    if (ev.type == KU_EVENT_KEY_DOWN)
    {
	vh_key_t*      vh    = view->evt_han_data;
	vh_key_event_t event = {.ev = ev, .vh = vh, .view = view};
	if (vh->on_event)
	    cancel = (*vh->on_event)(event);
    }

    return cancel;
}

void vh_key_del(void* p)
{
}

void vh_key_desc(void* p, int level)
{
    printf("vh_key");
}

void vh_key_add(ku_view_t* view, int (*on_event)(vh_key_event_t))
{
    assert(view->evt_han == NULL && view->evt_han_data == NULL);

    vh_key_t* vh = CAL(sizeof(vh_key_t), vh_key_del, vh_key_desc);
    vh->on_event = on_event;

    view->evt_han_data = vh;
    view->evt_han      = vh_key_evt;
}

#endif

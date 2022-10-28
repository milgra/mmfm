#ifndef ku_window_h
#define ku_window_h

#include "ku_bitmap.c"
#include "view.c"
#include "wm_event.c"

typedef struct _ku_window_t ku_window_t;
struct _ku_window_t
{
    view_t* root;
    vec_t*  views;

    vec_t* implqueue; // views selected by roll over
    vec_t* explqueue; // views selected by click
};

ku_window_t* window_create(int width, int height);
void         window_event(ku_window_t* window, ev_t event);
void         window_add(ku_window_t* window, view_t* view);
void         window_remove(ku_window_t* window, view_t* view);
void         window_render(ku_window_t* window, uint32_t time, bm_t* bm, vr_t dirty);
void         window_activate(ku_window_t* window, view_t* view);
view_t*      window_get_root(ku_window_t* window);
vr_t         window_update(ku_window_t* window, uint32_t time);
void         window_resize_to_root(ku_window_t* window, view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_layout.c"
#include "views.c"
#include "zc_log.c"
#include "zc_map.c"
#include "zc_util2.c"
#include "zc_vec2.c"
#include "zc_vector.c"

void window_del(void* p)
{
    ku_window_t* win = p;

    REL(win->root); // REL 0
    REL(win->views);
    REL(win->implqueue);
    REL(win->explqueue);

    // TODO wont work with multiple instances
    views_destroy();
}

ku_window_t* window_create(int width, int height)
{
    // TODO wont work with multiple instance
    views_init();

    ku_window_t* win = CAL(sizeof(ku_window_t), window_del, NULL);

    win->root      = view_new("root", (vr_t){0, 0, width, height}); // REL 0
    win->views     = VNEW();
    win->implqueue = VNEW();
    win->explqueue = VNEW();

    return win;
}

void window_event(ku_window_t* win, ev_t ev)
{
    if (ev.type == EV_TIME)
    {
	view_evt(win->root, ev);
    }
    else if (ev.type == EV_RESIZE)
    {
	vr_t rf = win->root->frame.local;
	if (rf.w != (float) ev.w ||
	    rf.h != (float) ev.h)
	{
	    view_set_frame(win->root, (vr_t){0.0, 0.0, (float) ev.w, (float) ev.h});
	    ku_layout(win->root);
#ifdef DEBUG
	    view_describe(win->root, 0);
#endif
	    view_evt(win->root, ev);
	}
    }
    else if (ev.type == EV_MMOVE)
    {
	ev_t outev = ev;
	outev.type = EV_MMOVE_OUT;
	for (int i = win->implqueue->length - 1; i > -1; i--)
	{
	    view_t* v = win->implqueue->data[i];
	    if (v->needs_touch)
	    {
		if (ev.x > v->frame.global.x &&
		    ev.x < v->frame.global.x + v->frame.global.w &&
		    ev.y > v->frame.global.y &&
		    ev.y < v->frame.global.y + v->frame.global.h)
		{
		}
		else
		{
		    if (v->handler) (*v->handler)(v, outev);
		    if (v->blocks_touch) break;
		}
	    }
	}

	vec_reset(win->implqueue);
	view_coll_touched(win->root, ev, win->implqueue);

	for (int i = win->implqueue->length - 1; i > -1; i--)
	{
	    view_t* v = win->implqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_touch) break;
	    }
	}
    }
    else if (ev.type == EV_MDOWN || ev.type == EV_MUP)
    {
	ev_t outev = ev;
	if (ev.type == EV_MDOWN) outev.type = EV_MDOWN_OUT;
	if (ev.type == EV_MUP) outev.type = EV_MUP_OUT;

	for (int i = win->explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = win->explqueue->data[i];
	    if (v->needs_touch)
	    {
		if (v->handler) (*v->handler)(v, outev);
		if (v->blocks_touch) break;
	    }
	}

	if (ev.type == EV_MDOWN)
	{
	    vec_reset(win->explqueue);
	    view_coll_touched(win->root, ev, win->explqueue);
	}

	for (int i = win->explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = win->explqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_touch) break;
	    }
	}
    }
    else if (ev.type == EV_SCROLL)
    {
	vec_reset(win->implqueue);
	view_coll_touched(win->root, ev, win->implqueue);

	for (int i = win->implqueue->length - 1; i > -1; i--)
	{
	    view_t* v = win->implqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_scroll) break;
	    }
	}
    }
    else if (ev.type == EV_KDOWN || ev.type == EV_KUP)
    {
	for (int i = win->explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = win->explqueue->data[i];

	    if (v->needs_key)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_key) break;
	    }
	}
    }
    else if (ev.type == EV_TEXT)
    {
	for (int i = win->explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = win->explqueue->data[i];
	    if (v->needs_text)
	    {
		if (v->handler) (*v->handler)(v, ev);
		break;
	    }
	}
    }
}

void window_add(ku_window_t* win, view_t* view)
{
    view_add_subview(win->root, view);

    // layout, window could be resized since
    ku_layout(win->root);
}

void window_remove(ku_window_t* win, view_t* view)
{
    view_remove_from_parent(view);
}

void window_activate(ku_window_t* win, view_t* view)
{
    vec_add_unique_data(win->explqueue, view);
}

void window_collect(ku_window_t* win, view_t* view, vec_t* views)
{
    if (!view->exclude) VADD(views, view);
    if (view->style.unmask == 1) view->style.unmask = 0; // reset unmasking
    vec_t* vec = view->views;
    for (int i = 0; i < vec->length; i++) window_collect(win, vec->data[i], views);
    if (view->style.masked)
    {
	view_t* last       = views->data[views->length - 1];
	last->style.unmask = 1;
    }
}

vr_t window_update(ku_window_t* win, uint32_t time)
{
    vr_t result = {0};

    if (views.arrange == 1)
    {
	vec_reset(win->views);
	window_collect(win, win->root, win->views);

	views.arrange = 0;
    }

    for (int i = 0; i < win->views->length; i++)
    {
	view_t* view = win->views->data[i];

	/* react to changes */

	if (view->texture.type == TT_MANAGED && view->texture.state == TS_BLANK) view_gen_texture(view);

	if (view->texture.changed)
	{
	    result                = vr_add(result, view->frame.global);
	    view->texture.changed = 0;
	}
	else if (view->frame.dim_changed)
	{
	    result                  = vr_add(result, view->frame.global);
	    view->frame.dim_changed = 0;
	}
	else if (view->frame.pos_changed)
	{
	    result                  = vr_add(result, view->frame.global);
	    view->frame.pos_changed = 0;
	}
	else if (view->frame.reg_changed)
	{
	    result                  = vr_add(result, view->frame.global);
	    view->frame.reg_changed = 0;
	}
	else if (view->texture.alpha_changed)
	{
	    result                      = vr_add(result, view->frame.global);
	    view->texture.alpha_changed = 0;
	}
    }

    return result;
}

void window_render(ku_window_t* win, uint32_t time, bm_t* bm, vr_t dirty)
{
    static vr_t masks[32] = {0};
    static int  maski     = 0;

    masks[0] = dirty;

    /* draw view into bitmap */

    for (int i = 0; i < win->views->length; i++)
    {
	view_t* view = win->views->data[i];

	if (view->style.masked)
	{
	    dirty = vr_is(masks[maski], view->frame.global);
	    maski++;
	    masks[maski] = dirty;
	    /* printf("%s masked, dirty %f %f %f %f\n", view->id, dirty.x, dirty.y, dirty.w, dirty.h); */
	}

	if (view->texture.bitmap)
	{
	    if (view->texture.transparent == 0 || i == 0)
	    {
		bm_insert(
		    bm,
		    view->texture.bitmap,
		    view->frame.global.x - view->style.shadow_blur,
		    view->frame.global.y - view->style.shadow_blur,
		    (int) dirty.x,
		    (int) dirty.y,
		    (int) dirty.w,
		    (int) dirty.h);
	    }
	    else
	    {
		if (view->texture.alpha == 1.0)
		{
		    bm_blend(
			bm,
			view->texture.bitmap,
			view->frame.global.x - view->style.shadow_blur,
			view->frame.global.y - view->style.shadow_blur,
			(int) dirty.x,
			(int) dirty.y,
			(int) dirty.w,
			(int) dirty.h);
		}
		else
		{
		    bm_blend_with_alpha_mask(
			bm,
			view->texture.bitmap,
			view->frame.global.x - view->style.shadow_blur,
			view->frame.global.y - view->style.shadow_blur,
			(int) dirty.x,
			(int) dirty.y,
			(int) dirty.w,
			(int) dirty.h,
			(255 - (int) (view->texture.alpha * 255.0)));
		}
	    }
	}

	if (view->style.unmask)
	{
	    maski--;
	    dirty = masks[maski];
	}
    }
}

view_t* window_get_root(ku_window_t* win)
{
    return win->root;
}

void window_resize_to_root(ku_window_t* win, view_t* view)
{
    vr_t f = win->root->frame.local;

    view_set_frame(view, (vr_t){0.0, 0.0, (float) f.w, (float) f.h});
    ku_layout(view);
}

#endif

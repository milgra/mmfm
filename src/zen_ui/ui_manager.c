#ifndef ui_manager_h
#define ui_manager_h

#include "view.c"
#include "wm_event.c"
#include "zc_bm_argb.c"

void    ui_manager_init(int width, int height);
void    ui_manager_destroy();
void    ui_manager_event(ev_t event);
void    ui_manager_add(view_t* view);
void    ui_manager_remove(view_t* view);
void    ui_manager_render(uint32_t time, bm_argb_t* bm);
void    ui_manager_activate(view_t* view);
view_t* ui_manager_get_root();
vr_t    ui_manager_update(uint32_t time);
void    ui_manager_resize_to_root(view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "view_layout.c"
#include "views.c"
#include "zc_log.c"
#include "zc_map.c"
#include "zc_util2.c"
#include "zc_vec2.c"
#include "zc_vector.c"

struct _uim_t
{
    view_t* root;
    vec_t*  views;

    vec_t* implqueue; // views selected by roll over
    vec_t* explqueue; // views selected by click

    vr_t dirty_rect;
    vr_t dirty_rect_prev;

} uim = {0};

void ui_manager_init(int width, int height)
{
    views_init();

    uim.root      = view_new("root", (vr_t){0, 0, width, height}); // REL 0
    uim.views     = VNEW();
    uim.implqueue = VNEW();
    uim.explqueue = VNEW();
}

void ui_manager_destroy()
{
    REL(uim.root); // REL 0
    REL(uim.views);
    REL(uim.implqueue);
    REL(uim.explqueue);

    views_destroy();
}

void ui_manager_event(ev_t ev)
{
    if (ev.type == EV_TIME)
    {
	view_evt(uim.root, ev);
    }
    else if (ev.type == EV_RESIZE)
    {
	vr_t rf = uim.root->frame.local;
	if (rf.w != (float) ev.w ||
	    rf.h != (float) ev.h)
	{
	    view_set_frame(uim.root, (vr_t){0.0, 0.0, (float) ev.w, (float) ev.h});
	    view_layout(uim.root);
#ifdef DEBUG
	    view_describe(uim.root, 0);
#endif
	    view_evt(uim.root, ev);
	}
    }
    else if (ev.type == EV_MMOVE)
    {
	ev_t outev = ev;
	outev.type = EV_MMOVE_OUT;
	for (int i = uim.implqueue->length - 1; i > -1; i--)
	{
	    view_t* v = uim.implqueue->data[i];
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

	vec_reset(uim.implqueue);
	view_coll_touched(uim.root, ev, uim.implqueue);

	for (int i = uim.implqueue->length - 1; i > -1; i--)
	{
	    view_t* v = uim.implqueue->data[i];
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

	for (int i = uim.explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = uim.explqueue->data[i];
	    if (v->needs_touch)
	    {
		if (v->handler) (*v->handler)(v, outev);
		if (v->blocks_touch) break;
	    }
	}

	if (ev.type == EV_MDOWN)
	{
	    vec_reset(uim.explqueue);
	    view_coll_touched(uim.root, ev, uim.explqueue);
	}

	for (int i = uim.explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = uim.explqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_touch) break;
	    }
	}
    }
    else if (ev.type == EV_SCROLL)
    {
	vec_reset(uim.implqueue);
	view_coll_touched(uim.root, ev, uim.implqueue);

	for (int i = uim.implqueue->length - 1; i > -1; i--)
	{
	    view_t* v = uim.implqueue->data[i];
	    if (v->needs_touch && v->parent)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_scroll) break;
	    }
	}
    }
    else if (ev.type == EV_KDOWN || ev.type == EV_KUP)
    {
	for (int i = uim.explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = uim.explqueue->data[i];

	    if (v->needs_key)
	    {
		if (v->handler) (*v->handler)(v, ev);
		if (v->blocks_key) break;
	    }
	}
    }
    else if (ev.type == EV_TEXT)
    {
	for (int i = uim.explqueue->length - 1; i > -1; i--)
	{
	    view_t* v = uim.explqueue->data[i];
	    if (v->needs_text)
	    {
		if (v->handler) (*v->handler)(v, ev);
		break;
	    }
	}
    }
}

void ui_manager_add(view_t* view)
{
    view_add_subview(uim.root, view);

    // layout, window could be resized since
    view_layout(uim.root);
}

void ui_manager_remove(view_t* view)
{
    view_remove_from_parent(view);
}

void ui_manager_activate(view_t* view)
{
    vec_add_unique_data(uim.explqueue, view);
}

void ui_manager_collect(view_t* view, vec_t* views)
{
    if (!view->exclude) VADD(views, view);
    if (view->style.unmask == 1) view->style.unmask = 0; // reset unmasking
    vec_t* vec = view->views;
    for (int i = 0; i < vec->length; i++) ui_manager_collect(vec->data[i], views);
    if (view->style.masked)
    {
	view_t* last       = views->data[views->length - 1];
	last->style.unmask = 1;
    }
}

vr_t ui_manager_update(uint32_t time)
{
    vr_t result = {0};

    uim.dirty_rect.w = 0;
    uim.dirty_rect.h = 0;

    if (views.arrange == 1)
    {
	vec_reset(uim.views);
	ui_manager_collect(uim.root, uim.views);

	views.arrange = 0;
    }

    for (int i = 0; i < uim.views->length; i++)
    {
	view_t* view = uim.views->data[i];

	/* react to changes */

	if (view->texture.type == TT_MANAGED && view->texture.state == TS_BLANK)
	{
	    view_gen_texture(view);
	}

	if (view->texture.changed)
	{
	    uim.dirty_rect = vr_add(uim.dirty_rect, view->frame.global);

	    view->texture.changed = 0;
	}

	if (view->frame.dim_changed)
	{
	    uim.dirty_rect = vr_add(uim.dirty_rect, view->frame.global);

	    view->frame.dim_changed = 0;
	}

	if (view->frame.pos_changed)
	{
	    uim.dirty_rect = vr_add(uim.dirty_rect, view->frame.global);

	    view->frame.pos_changed = 0;
	}

	if (view->frame.reg_changed)
	{
	    view->frame.reg_changed = 0;
	}

	if (view->texture.alpha_changed)
	{
	    view->texture.alpha_changed = 0;
	}
    }

    // final dirty rect is the sum of the prev dirty rect and the actual dirty rect

    if (uim.dirty_rect.w > 0 || uim.dirty_rect_prev.w > 0)
    {
	result = vr_add(uim.dirty_rect_prev, uim.dirty_rect);
	printf("dirty %f %f %f %f\n", uim.dirty_rect.x, uim.dirty_rect.y, uim.dirty_rect.w, uim.dirty_rect.h);
    }

    uim.dirty_rect_prev = uim.dirty_rect;

    return result;
}

void ui_manager_render(uint32_t time, bm_argb_t* bm)
{
    /* draw view into bitmap */

    for (int i = 0; i < uim.views->length; i++)
    {
	view_t* view = uim.views->data[i];

	if (view->texture.bitmap)
	{
	    if (view->texture.transparent == 0 || i == 0)
		bm_argb_insert(
		    bm,
		    view->texture.bitmap,
		    view->frame.global.x - view->style.shadow_blur,
		    view->frame.global.y - view->style.shadow_blur);
	    else
	    {
		if (view->texture.alpha == 1.0)
		    bm_argb_blend(
			bm,
			view->texture.bitmap,
			view->frame.global.x - view->style.shadow_blur,
			view->frame.global.y - view->style.shadow_blur);
		else
		    bm_argb_blend_with_alpha_mask(
			bm,
			view->texture.bitmap,
			view->frame.global.x - view->style.shadow_blur,
			view->frame.global.y - view->style.shadow_blur,
			(255 - (int) (view->texture.alpha * 255.0)));
	    }
	}
    }
}

view_t* ui_manager_get_root()
{
    return uim.root;
}

void ui_manager_resize_to_root(view_t* view)
{
    vr_t f = uim.root->frame.local;

    view_set_frame(view, (vr_t){0.0, 0.0, (float) f.w, (float) f.h});
    view_layout(view);
}

#endif

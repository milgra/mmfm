/*
  CSS texture generator
  Generates texture based on css style
 */

#ifndef texgen_css_h
#define texgen_css_h

#include "view.c"
#include "zc_bm_rgba.c"

typedef struct _tg_css_t
{
    char*      id;
    char*      path;
    bm_rgba_t* bitmap;
} tg_bitmap_t;

void tg_css_add(view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "coder.c"
#include "zc_cstring.c"
#include "zc_draw.c"
#include "zc_log.c"

uint32_t tg_css_graycolor = 0;

void tg_css_gen(view_t* view)
{
    if (view->frame.local.w >= 1.0 &&
	view->frame.local.h >= 1.0)
    {
	if (view->style.background_image)
	{
	    bm_rgba_t* bm = view->texture.bitmap;

	    if (bm == NULL ||
		bm->w != (int) view->frame.local.w ||
		bm->h != (int) view->frame.local.h)
	    {
		bm = bm_rgba_new((int) view->frame.local.w, (int) view->frame.local.h); // REL 0
		view_set_texture_bmp(view, bm);
		REL(bm);
	    }

	    coder_load_image_into(view->style.background_image, view->texture.bitmap);
	    view->texture.changed = 0;
	    view->texture.state   = TS_READY;

	    /* bm_rgba_t* bmap = coder_get_image(view->style.background_image); */
	    /* view_set_texture_bmp(view, bmap); */
	    /* REL(bmap); */
	}
	else if (view->style.background_color)
	{
	    uint32_t color = view->style.background_color;

	    float w = view->frame.local.w + 2 * view->style.shadow_blur;
	    float h = view->frame.local.h + 2 * view->style.shadow_blur;

	    bm_rgba_t* bm = view->texture.bitmap;

	    if (bm == NULL ||
		bm->w != (int) w ||
		bm->h != (int) h)
	    {
		bm = bm_rgba_new((int) w, (int) h); // REL 0
		view_set_texture_bmp(view, bm);
		REL(bm);
	    }

	    bm_rgba_reset(bm);

	    gfx_rounded_rect(bm, 0, 0, w, h, view->style.border_radius, view->style.shadow_blur, color, view->style.shadow_color);

	    view->texture.changed = 1;
	    view->texture.state   = TS_READY;
	}
    }
}

void tg_css_add(view_t* view)
{
    if (view->tex_gen != NULL) zc_log_debug("Text generator already exist for view, cannot create a new one : %s", view->id);
    else
    {
	view->tex_gen = tg_css_gen;
	view->exclude = 0;
    }
}

#endif

#ifndef ku_renderer_egl_h
#define ku_renderer_egl_h

#include "ku_bitmap.c"
#include "zc_vector.c"

void ku_renderer_egl_init();
void ku_renderer_egl_render(vec_t* views, ku_bitmap_t* bitmap);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_gl.c"
#include "ku_rect.c"
#include "ku_view.c"
#include "zc_time.c"

void ku_renderer_egl_init()
{
    ku_gl_init();
}
void ku_renderer_egl_render(vec_t* views, ku_bitmap_t* bitmap)
{
    /* calculate dirty rect */

    ku_rect_t dirty = {0};

    for (int i = 0; i < views->length; i++)
    {
	ku_view_t* view = views->data[i];

	if (view->texture.type == TT_MANAGED && view->texture.state == TS_BLANK) ku_view_gen_texture(view);

	if (view->texture.changed)
	{
	    dirty                 = ku_rect_add(dirty, view->frame.global);
	    view->texture.changed = 0;
	}
	else if (view->frame.dim_changed)
	{
	    dirty                   = ku_rect_add(dirty, view->frame.global);
	    view->frame.dim_changed = 0;
	}
	else if (view->frame.pos_changed)
	{
	    dirty                   = ku_rect_add(dirty, view->frame.global);
	    view->frame.pos_changed = 0;
	}
	else if (view->frame.reg_changed)
	{
	    dirty                   = ku_rect_add(dirty, view->frame.global);
	    view->frame.reg_changed = 0;
	}
	else if (view->texture.alpha_changed)
	{
	    dirty                       = ku_rect_add(dirty, view->frame.global);
	    view->texture.alpha_changed = 0;
	}
    }

    zc_time(NULL);
    ku_gl_add_textures(views);
    zc_time("texture");
    ku_gl_add_vertexes(views);
    zc_time("vertex");
    ku_gl_render(bitmap);
    zc_time("render");

    /* draw views */

    /* if (view->texture.bitmap) */
    /* { */
    /* 	ku_rect_t rect = view->frame.global; */

    /* 	bmr_t dstmsk = ku_bitmap_is( */
    /* 	    (bmr_t){(int) dirty.x, (int) dirty.y, (int) (dirty.x + dirty.w), (int) (dirty.y + dirty.h)}, */
    /* 	    (bmr_t){0, 0, bm->w, bm->h}); */

    /* 	bmr_t srcmsk = {0, 0, view->texture.bitmap->w, view->texture.bitmap->h}; */

    /* 	/\* if there is a region to draw, modify source mask *\/ */

    /* 	if (view->frame.region.w > 0 && view->frame.region.h > 0) */
    /* 	{ */
    /* 	    srcmsk.x += view->frame.region.x; */
    /* 	    srcmsk.y += view->frame.region.y; */
    /* 	    srcmsk.z = srcmsk.x + view->frame.region.w; */
    /* 	    srcmsk.w = srcmsk.y + view->frame.region.h; */
    /* 	} */

    /* 	/\* draw with shadow blur outlets in mind *\/ */

    /* 	if (view->texture.transparent == 0 || i == 0) */
    /* 	{ */
    /* 	    ku_bitmap_insert( */
    /* 		bm, */
    /* 		dstmsk, */
    /* 		view->texture.bitmap, */
    /* 		srcmsk, */
    /* 		rect.x - view->style.shadow_blur, */
    /* 		rect.y - view->style.shadow_blur); */
    /* 	} */
    /* 	else */
    /* 	{ */
    /* 	    if (view->texture.alpha == 1.0) */
    /* 	    { */
    /* 		ku_bitmap_blend( */
    /* 		    bm, */
    /* 		    dstmsk, */
    /* 		    view->texture.bitmap, */
    /* 		    srcmsk, */
    /* 		    rect.x - view->style.shadow_blur, */
    /* 		    rect.y - view->style.shadow_blur); */
    /* 	    } */
    /* 	    else */
    /* 	    { */
    /* 		ku_bitmap_blend_with_alpha( */
    /* 		    bm, */
    /* 		    dstmsk, */
    /* 		    view->texture.bitmap, */
    /* 		    srcmsk, */
    /* 		    rect.x - view->style.shadow_blur, */
    /* 		    rect.y - view->style.shadow_blur, */
    /* 		    (255 - (int) (view->texture.alpha * 255.0))); */
    /* 	    } */
    /* 	} */
    /* } */

    /* if (view->style.unmask) */
    /* { */
    /* 	maski--; */
    /* 	dirty = masks[maski]; */
    /* } */
}

#endif

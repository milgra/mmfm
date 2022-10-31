/*
  CSS texture generator
  Generates texture based on css style
 */

#ifndef texgen_scaledimg_h
#define texgen_scaledimg_h

#include "ku_bitmap.c"
#include "ku_view.c"

typedef struct _tg_scaledimg_t
{
    ku_bitmap_t* bitmap;
    int          w;
    int          h;
} tg_scaledimg_t;

void tg_scaledimg_add(ku_view_t* view, int w, int h);
void tg_scaledimg_gen(ku_view_t* view);
void tg_scaledimg_set_content_size(ku_view_t* view, int w, int h);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "ku_draw.c"
#include "zc_cstring.c"
#include "zc_log.c"

void tg_scaledimg_gen(ku_view_t* view)
{
    tg_scaledimg_t* gen = view->tex_gen_data;
    ku_bitmap_t*    bm  = view->texture.bitmap;

    /* just resize texture bitmap with the view */

    if (bm == NULL ||
	bm->w != (int) view->frame.global.w ||
	bm->h != (int) view->frame.global.h)
    {
	bm = ku_bitmap_new((int) view->frame.global.w, (int) view->frame.global.h); // REL 0

	ku_view_set_texture_bmp(view, bm);

	/* if we have a bitmap, re-scale that */

	printf("GEN BITMAP %i\n", gen->bitmap == NULL);
	if (gen->bitmap)
	{
	    /* memset(bm->data, 0xFF, bm->size); */
	    ku_draw_scale(gen->bitmap, bm);
	}

	/* else it will be filled up externally */

	REL(bm);
    }

    view->texture.state = TS_READY;
}

void tg_scaledimg_set_content_size(ku_view_t* view, int w, int h)
{
    tg_scaledimg_t* gen = view->tex_gen_data;

    gen->w = w;
    gen->h = h;

    if (gen->bitmap) REL(gen->bitmap);

    gen->bitmap = ku_bitmap_new(w, h);
}

void tg_scaledimg_add(ku_view_t* view, int w, int h)
{
    assert(view->tex_gen == NULL);

    tg_scaledimg_t* gen = CAL(sizeof(tg_scaledimg_t), NULL, NULL);
    gen->w              = w;
    gen->h              = h;

    view->tex_gen_data = gen;
    view->tex_gen      = tg_scaledimg_gen;
    view->exclude      = 0;
}

#endif

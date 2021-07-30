/*
  CSS texture generator
  Generates texture based on css layout
 */

#ifndef texgen_css_h
#define texgen_css_h

#include "view.c"
#include "zc_bitmap.c"

typedef struct _tg_css_t
{
  char* id;
  char* path;
  bm_t* bitmap;
} tg_bitmap_t;

void tg_css_add(view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "coder.c"
#include "zc_cstring.c"
#include "zc_graphics.c"

uint32_t tg_css_graycolor = 0;

void tg_css_gen(view_t* view)
{
  if (view->frame.local.w >= 1.0 &&
      view->frame.local.h >= 1.0)
  {
    if (view->layout.background_image)
    {
      bm_t* bm = view->texture.bitmap;

      if (bm == NULL ||
          bm->w != (int)view->frame.local.w ||
          bm->h != (int)view->frame.local.h)
      {
        bm = bm_new((int)view->frame.local.w, (int)view->frame.local.h); // REL 0
        view_set_texture_bmp(view, bm);
        REL(bm);
      }

      coder_load_image_into(view->layout.background_image, view->texture.bitmap);
      view->texture.changed = 0;
      view->texture.state   = TS_READY;

      /* bm_t* bmap = coder_get_image(view->layout.background_image); */
      /* view_set_texture_bmp(view, bmap); */
      /* REL(bmap); */
    }
    else if (view->layout.background_color)
    {
      uint32_t color = view->layout.background_color;

      float w = view->frame.local.w + 2 * view->layout.shadow_blur;
      float h = view->frame.local.h + 2 * view->layout.shadow_blur;

      bm_t* bm = view->texture.bitmap;

      if (bm == NULL ||
          bm->w != (int)w ||
          bm->h != (int)h)
      {
        bm = bm_new((int)w, (int)h); // REL 0
        view_set_texture_bmp(view, bm);
        REL(bm);
      }

      bm_reset(bm);

      gfx_rounded_rect(bm,
                       0,
                       0,
                       w,
                       h,
                       view->layout.border_radius,
                       view->layout.shadow_blur,
                       color,
                       view->layout.shadow_color);

      view->texture.changed = 1;
    }
  }
}

void tg_css_add(view_t* view)
{
  assert(view->tex_gen == NULL);

  view->tex_gen = tg_css_gen;
  view->exclude = 0;
}

#endif

#ifndef ui_util_h
#define ui_util_h

#include "view.c"
#include "zc_text.c"

textstyle_t ui_util_gen_textstyle(view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "fontconfig.c"
#include <limits.h>

textstyle_t ui_util_gen_textstyle(view_t* view)
{
  textstyle_t style = {0};

  style.font = fontconfig_new_path(view->layout.font_family);
  style.size = view->layout.font_size;

  style.align = view->layout.text_align;
  /* style.valign = */
  /* style.autosize = */
  style.multiline   = view->layout.word_wrap == 1;
  style.line_height = view->layout.line_height;

  style.margin = view->layout.margin;
  if (view->layout.margin_left < INT_MAX) style.margin_left = view->layout.margin_left;
  if (view->layout.margin_right < INT_MAX) style.margin_right = view->layout.margin_right;
  if (view->layout.margin_top < INT_MAX) style.margin_top = view->layout.margin_top;
  if (view->layout.margin_bottom < INT_MAX) style.margin_bottom = view->layout.margin_bottom;

  style.textcolor = view->layout.color;
  style.backcolor = view->layout.background_color;

  return style;
}

#endif

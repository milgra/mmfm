#ifndef ui_about_popup_h
#define ui_about_popup_h

#include "view.c"
#include "zc_map.c"

void ui_about_popup_attach(view_t* base);
void ui_about_popup_detach();
void ui_about_popup_update();
void ui_about_popup_refresh();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "column.c"
#include "config.c"
#include "selection.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_alert_popup.c"
#include "vh_button.c"
#include "vh_list.c"
#include "vh_list_head.c"
#include "vh_list_item.c"
#include "visible.c"
#include <limits.h>

void ui_about_popup_on_header_field_select(view_t* view, char* id, ev_t ev);
void ui_about_popup_on_header_field_insert(view_t* view, int src, int tgt);
void ui_about_popup_on_header_field_resize(view_t* view, char* id, int size);

struct ui_about_popup_tv
{
  view_t*     view;   // table view
  vec_t*      fields; // fileds in table
  vec_t*      items;  // table items
  textstyle_t textstyle;
  void (*popup)(char* text);
} donl = {0};

void ui_about_popup_update()
{
  vh_list_reset(donl.view);
}

void ui_about_popup_refresh()
{
  vh_list_refresh(donl.view);
}

void ui_about_popup_on_header_field_select(view_t* view, char* id, ev_t ev)
{
  // (*donl.on_header_select)(id);
}

void ui_about_popup_on_header_field_insert(view_t* view, int src, int tgt)
{
  // update in fields so new items will use updated order
  col_t* cell = donl.fields->data[src];

  RET(cell); // REL 0
  VREM(donl.fields, cell);
  vec_ins(donl.fields, cell, tgt);
  REL(cell); // REL 0

  // update all items and cache
  vec_t* items = vh_list_items(donl.view);
  for (int i = 0; i < items->length; i++)
  {
    view_t* item = items->data[i];
    vh_litem_swp_cell(item, src, tgt);
  }
}

void ui_about_popup_on_header_field_resize(view_t* view, char* id, int size)
{
  // update in fields so new items will use updated size
  for (int i = 0; i < donl.fields->length; i++)
  {
    col_t* cell = donl.fields->data[i];
    if (strcmp(cell->id, id) == 0)
    {
      cell->size = size;
      break;
    }
  }

  // update all items and cache
  vec_t* items = vh_list_items(donl.view);
  for (int i = 0; i < items->length; i++)
  {
    view_t* item = items->data[i];
    vh_litem_upd_cell_size(item, id, size);
  }
}

// items

void ui_about_popup_on_item_select(view_t* itemview, int index, vh_lcell_t* cell, ev_t ev)
{
  switch (index)
  {
  case 1:
    system("xdg-open https://patreon.com/milgra");
    break;
  case 2:
    system("xdg-open https://paypal.me/milgra");
    break;
  case 3:
    system("xdg-open https://github.com/milgra/zenmusic/issues");
    break;
  case 4:
    system("xdg-open https://github.com/milgra/zenmusic");
    break;
  }

  if (index < 5) ui_alert_popup_show("Link is opened in the browser.");
}

view_t* donateitem_new(int index)
{
  static int item_cnt      = 0;
  char       idbuffer[100] = {0};
  snprintf(idbuffer, 100, "donlist_item%i", item_cnt++);

  float height = 50;
  if (index == 0) height = 130;
  if (index == 5) height = 160;

  view_t* rowview = view_new(idbuffer, (r2_t){0, 0, 470, height}); // REL 0

  vh_litem_add(rowview, NULL);
  vh_litem_set_on_select(rowview, ui_about_popup_on_item_select);

  for (int i = 0; i < donl.fields->length; i++)
  {
    col_t*  cell     = donl.fields->data[i];
    char*   id       = cstr_new_format(100, "%s%s", rowview->id, cell->id); // REL 1
    view_t* cellview = view_new(id, (r2_t){0, 0, cell->size, height});      // REL 2

    if (index == 5)
    {
      view_t* imgview                  = view_new("bsdlogo", ((r2_t){137, 10, 200, 200})); // REL 3
      char*   respath                  = config_get("res_path");
      char*   imagepath                = cstr_new_format(100, "%s/freebsd.png", respath); // REL 4
      imgview->layout.background_image = imagepath;
      tg_css_add(imgview);
      view_add_subview(cellview, imgview);

      REL(imgview);

      vh_litem_upd_index(rowview, 5);
    }
    else
    {
      tg_text_add(cellview);
    }

    vh_litem_add_cell(rowview, cell->id, cell->size, cellview);

    REL(id);       // REL 1
    REL(cellview); // REL 2
  }

  return rowview;
}

void donateitem_update_row(view_t* rowview, int index, char* field)
{
  uint32_t color1          = (index % 2 == 0) ? 0xEFEFEFFF : 0xE5E5E5FF;
  donl.textstyle.backcolor = color1;

  vh_litem_upd_index(rowview, index);

  tg_text_set(vh_litem_get_cell(rowview, "field"), field, donl.textstyle);
}

view_t* ui_about_popup_item_for_index(int index, void* userdata, view_t* listview, int* item_count)
{
  if (index < 0)
    return NULL; // no items before 0
  if (index >= donl.items->length)
    return NULL; // no more items

  *item_count = donl.items->length;

  return donl.items->data[index];
}

void ui_about_popup_attach(view_t* baseview)
{
  donl.view   = view_get_subview(baseview, "aboutlist");
  donl.fields = VNEW(); // REL 0
  donl.items  = VNEW(); // REL 1

  donl.textstyle.font      = config_get("font_path");
  donl.textstyle.align     = TA_CENTER;
  donl.textstyle.margin    = 10;
  donl.textstyle.size      = 30.0;
  donl.textstyle.textcolor = 0x000000FF;
  donl.textstyle.backcolor = 0xF5F5F5FF;
  donl.textstyle.multiline = 1;

  // create fields

  VADD(donl.fields, col_new("field", 470, 0));

  vec_dec_retcount(donl.fields);

  // add list handler to view

  vh_list_add(donl.view, ((vh_list_inset_t){0, 0, 0, 0}), ui_about_popup_item_for_index, NULL, NULL);

  // create items

  VADDR(donl.items, donateitem_new(0));
  VADDR(donl.items, donateitem_new(1));
  VADDR(donl.items, donateitem_new(2));
  VADDR(donl.items, donateitem_new(3));
  VADDR(donl.items, donateitem_new(4));
  VADDR(donl.items, donateitem_new(5));

  // char* version = cstr_new_format(200, "Zen Music v%i.%i beta\nby Milan Toth\nFree and Open Source Software.", VERSION, BUILD); // REL 2

  char* version = cstr_new_format(200, "MultiMedia File Manager v%s beta\nby Milan Toth\nFree and Open Source Software.", MMFM_VERSION); // REL 2

  donateitem_update_row(donl.items->data[0], 0, version);
  donateitem_update_row(donl.items->data[1], 1, "Support on Patreon");
  donateitem_update_row(donl.items->data[2], 2, "Donate on Paypal");
  donateitem_update_row(donl.items->data[3], 3, "Report an Issue");
  donateitem_update_row(donl.items->data[4], 4, "GitHub Page");

  REL(version); // REL 2
}

void ui_about_popup_detach()
{
  REL(donl.fields); // REL 0
  REL(donl.items);  // REL 1
}

#endif

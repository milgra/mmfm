#ifndef ui_settings_popup_h
#define ui_settings_popup_h

#include "view.c"
#include "zc_map.c"

void ui_settings_popup_attach(view_t* view);
void ui_settings_popup_detach();
void ui_settings_popup_update();
void ui_settings_popup_refresh();
void ui_settings_popup_show();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "callbacks.c"
#include "column.c"
#include "config.c"
#include "selection.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_alert_popup.c"
#include "ui_decision_popup.c"
#include "ui_inputfield_popup.c"
#include "ui_popup_switcher.c"
#include "vh_list.c"
#include "vh_list_head.c"
#include "vh_list_item.c"
#include "zc_callback.c"

void ui_settings_popup_on_header_field_select(view_t* view, char* id, ev_t ev);
void ui_settings_popup_on_header_field_insert(view_t* view, int src, int tgt);
void ui_settings_popup_on_header_field_resize(view_t* view, char* id, int size);

view_t* ui_settings_popup_item_for_index(int index, void* userdata, view_t* listview, int* item_count);
view_t* ui_settings_popup_new_item();
void    ui_settings_popup_update_item(view_t* rowview, int index, char* field, char* value);

struct ui_settings_popup_t
{
  view_t*     view;    // table view
  vec_t*      columns; // columns in table
  vec_t*      items;
  textstyle_t textstyle;
} uisp = {0};

void ui_settings_popup_attach(view_t* baseview)
{
  uisp.view    = view_get_subview(baseview, "settingslist");
  uisp.columns = VNEW(); // REL 0
  uisp.items   = VNEW(); // REL 1

  uisp.textstyle.font        = config_get("font_path");
  uisp.textstyle.align       = 0;
  uisp.textstyle.margin_left = 10;
  uisp.textstyle.size        = 30.0;
  uisp.textstyle.textcolor   = 0x000000FF;
  uisp.textstyle.backcolor   = 0xF5F5F5FF;

  // create columns

  VADDR(uisp.columns, col_new("key", 200, 0));
  VADDR(uisp.columns, col_new("value", 800, 1));

  // add header handler

  view_t* header = view_new("settingslist_header", (r2_t){0, 0, 10, 30}); // REL 2
  /* header->layout.background_color = 0x333333FF; */
  /* header->layout.shadow_blur      = 3; */
  /* header->layout.border_radius    = 3; */
  tg_css_add(header);

  vh_lhead_add(header);
  vh_lhead_set_on_select(header, ui_settings_popup_on_header_field_select);
  vh_lhead_set_on_insert(header, ui_settings_popup_on_header_field_insert);
  vh_lhead_set_on_resize(header, ui_settings_popup_on_header_field_resize);

  for (int i = 0; i < uisp.columns->length; i++)
  {
    col_t*  cell     = uisp.columns->data[i];
    char*   id       = cstr_new_format(100, "%s%s", header->id, cell->id); // REL 3
    view_t* cellview = view_new(id, (r2_t){0, 0, cell->size, 30});         // REL 4

    tg_text_add(cellview);
    tg_text_set(cellview, cell->id, uisp.textstyle);

    vh_lhead_add_cell(header, cell->id, cell->size, cellview);

    REL(id);       // REL 3
    REL(cellview); // REL 4
  }

  // add list handler to view

  vh_list_add(uisp.view, ((vh_list_inset_t){30, 0, 0, 0}), ui_settings_popup_item_for_index, NULL, NULL);
  vh_list_set_header(uisp.view, header);

  // create items

  VADDR(uisp.items, ui_settings_popup_new_item());
  VADDR(uisp.items, ui_settings_popup_new_item());
  // VADD(uisp.items, ui_settings_popup_new_item());
  VADDR(uisp.items, ui_settings_popup_new_item());
  VADDR(uisp.items, ui_settings_popup_new_item());
  VADDR(uisp.items, ui_settings_popup_new_item());

  ui_settings_popup_update_item(uisp.items->data[0], 0, "Library Path", "/home/user/milgra/Music");
  ui_settings_popup_update_item(uisp.items->data[1], 1, "Organize Library", "Disabled");
  //  ui_settings_popup_update_item(uisp.items->data[2], 2, "Dark Mode", "Disabled");
  ui_settings_popup_update_item(uisp.items->data[2], 2, "Remote Control", "Disabled");
  ui_settings_popup_update_item(uisp.items->data[3], 3, "Config Path", "/home/.config/zenmusic/config");
  ui_settings_popup_update_item(uisp.items->data[4], 4, "Style Path", "/usr/local/share/zenmusic");

  REL(header);
}

void ui_settings_popup_detach()
{
  REL(uisp.columns);
  REL(uisp.items);
}

void ui_settings_popup_show()
{
  ui_settings_popup_update_item(uisp.items->data[0], 0, "Library Path", config_get("lib_path"));
  ui_settings_popup_update_item(uisp.items->data[1], 1, "Organize Library", config_get("organize_lib"));
  //  ui_settings_popup_update_item(uisp.items->data[2], 2, "Dark Mode", config_get("dark_mode"));
  ui_settings_popup_update_item(uisp.items->data[2], 2, "Remote Control", config_get("remote_enabled"));
  ui_settings_popup_update_item(uisp.items->data[3], 3, "Config Path", config_get("cfg_path"));
  ui_settings_popup_update_item(uisp.items->data[4], 4, "HTML/Style Path", config_get("res_path"));

  ui_popup_switcher_toggle("settings_popup_page");
}

void ui_settings_popup_update()
{
  vh_list_reset(uisp.view);
}

void ui_settings_popup_refresh()
{
  vh_list_refresh(uisp.view);
}

void ui_settings_popup_on_header_field_select(view_t* view, char* id, ev_t ev)
{
  // (*uisp.on_header_select)(id);
}

void ui_settings_popup_on_header_field_insert(view_t* view, int src, int tgt)
{
  // update in columns so new items will use updated order
  col_t* cell = uisp.columns->data[src];

  RET(cell);
  VREM(uisp.columns, cell);
  vec_ins(uisp.columns, cell, tgt);
  REL(cell);

  // update all items and cache
  vec_t* items = vh_list_items(uisp.view);
  for (int i = 0; i < items->length; i++)
  {
    view_t* item = items->data[i];
    vh_litem_swp_cell(item, src, tgt);
  }
}

void ui_settings_popup_on_header_field_resize(view_t* view, char* id, int size)
{
  // update in columns so new items will use updated size
  for (int i = 0; i < uisp.columns->length; i++)
  {
    col_t* cell = uisp.columns->data[i];
    if (strcmp(cell->id, id) == 0)
    {
      cell->size = size;
      break;
    }
  }

  // update all items and cache
  vec_t* items = vh_list_items(uisp.view);
  for (int i = 0; i < items->length; i++)
  {
    view_t* item = items->data[i];
    vh_litem_upd_cell_size(item, id, size);
  }
}

void ui_settings_popup_on_accept(void* userdata, void* data)
{
  int enabled = config_get_bool("organize_lib");
  if (enabled)
    config_set_bool("organize_lib", 0);
  else
    config_set_bool("organize_lib", 1);
  config_write(config_get("cfg_path"));
  ui_settings_popup_update_item(uisp.items->data[1], 1, "Organize Library", config_get("organize_lib"));
  callbacks_call("on_change_organize", NULL);
}

void ui_settings_popup_on_accept_remote(void* userdata, void* data)
{
  int enabled = config_get_bool("remote_enabled");
  if (enabled)
    config_set_bool("remote_enabled", 0);
  else
    config_set_bool("remote_enabled", 1);
  config_write(config_get("cfg_path"));
  ui_settings_popup_update_item(uisp.items->data[2], 1, "Organize Library", config_get("remote_enabled"));
  callbacks_call("on_change_remote", NULL);
}

void ui_settings_popup_on_accept_library(void* userdata, void* data)
{
  char* path_cs = str_new_cstring(data); // REL 0

  callbacks_call("on_change_library", path_cs);
  REL(path_cs); // REL 0
}

// items

void ui_settings_popup_on_item_select(view_t* itemview, int index, vh_lcell_t* cell, ev_t ev)
{
  printf("item select %i\n", index);

  // ui_popup_switcher_toggle("decision_popup_page");

  switch (index)
  {
  case 0:
  {
    cb_t* acc_cb = cb_new(ui_settings_popup_on_accept_library, NULL); // REL 0
    ui_inputfield_popup_show("Use library at", acc_cb, NULL);
    REL(acc_cb); // REL 0

    break;
  }
  case 1:
  {
    int   enabled = config_get_bool("organize_lib");
    cb_t* acc_cb  = cb_new(ui_settings_popup_on_accept, NULL); // REL 0
    if (enabled)
      ui_decision_popup_show("Are you sure you want to switch off automatic organization?", acc_cb, NULL);
    else
      ui_decision_popup_show("Music files will be renamed/reorganized automatically under your library folder, based on artist, title and track number. Are you sure?", acc_cb, NULL);
    REL(acc_cb); // REL 0
    break;
  }
  case 2:
  {
    int   enabled = config_get_bool("remote_enabled");
    cb_t* acc_cb  = cb_new(ui_settings_popup_on_accept_remote, NULL); // REL 0
    if (enabled)
      ui_decision_popup_show("Are you sure you want to switch off remote control?", acc_cb, NULL);
    else
    {
      char* message = cstr_new_format(200, "You can remote control Zen Music by sending 0x00(play/pause) 0x01(prev song) 0x02(next song) to UDP port %s. "
                                           "Would you like to enable it?",
                                      config_get("remote_port"));
      ui_decision_popup_show(message, acc_cb, NULL);
      REL(message);
    }
    REL(acc_cb); // REL 0
    break;
  }
  case 3:
    ui_alert_popup_show("You cannot set the config path");
    break;
  case 4:
    ui_alert_popup_show("You cannot set the style path");
    break;
  }
}

view_t* ui_settings_popup_new_item()
{
  static int item_cnt      = 0;
  char       idbuffer[100] = {0};
  snprintf(idbuffer, 100, "uispist_item%i", item_cnt++);

  view_t* rowview = view_new(idbuffer, (r2_t){0, 0, 1000, 50}); // REL 0

  vh_litem_add(rowview, NULL);
  vh_litem_set_on_select(rowview, ui_settings_popup_on_item_select);

  for (int i = 0; i < uisp.columns->length; i++)
  {
    col_t*  cell     = uisp.columns->data[i];
    char*   id       = cstr_new_format(100, "%s%s", rowview->id, cell->id); // REL 1
    view_t* cellview = view_new(id, (r2_t){0, 0, cell->size, 50});          // REL 2

    tg_text_add(cellview);

    vh_litem_add_cell(rowview, cell->id, cell->size, cellview);

    REL(id);
    REL(cellview);
  }

  return rowview;
}

void ui_settings_popup_update_item(view_t* rowview, int index, char* field, char* value)
{
  uint32_t color1          = (index % 2 == 0) ? 0xEFEFEFFF : 0xE5E5E5FF;
  uisp.textstyle.backcolor = color1;

  vh_litem_upd_index(rowview, index);

  tg_text_set(vh_litem_get_cell(rowview, "key"), field, uisp.textstyle);
  tg_text_set(vh_litem_get_cell(rowview, "value"), value, uisp.textstyle);
}

view_t* ui_settings_popup_item_for_index(int index, void* userdata, view_t* listview, int* item_count)
{
  if (index < 0)
    return NULL; // no items before 0
  if (index >= uisp.items->length)
    return NULL; // no more items

  *item_count = uisp.items->length;

  return uisp.items->data[index];
}

#endif

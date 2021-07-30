#ifndef ui_song_menu_popup_h
#define ui_song_menu_popup_h

#include "view.c"
#include "zc_map.c"

void ui_song_menu_popup_attach(view_t* view);
void ui_song_menu_popup_detach();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "database.c"
#include "library.c"
#include "text.c"
#include "tg_text.c"
#include "ui_decision_popup.c"
#include "ui_editor_popup.c"
#include "ui_play_controls.c"
#include "ui_popup_switcher.c"
#include "ui_songlist.c"
#include "vh_list.c"
#include "vh_list_item.c"
#include "visible.c"

struct ui_song_menu_popup_t
{
  view_t*     view; // table view
  vec_t*      items;
  textstyle_t textstyle;
} slp = {0};

view_t* ui_song_menu_popup_item_for_index(int index, void* userdata, view_t* listview, int* item_count);
view_t* ui_song_menu_popupitem_new(int index, char* text);

void ui_song_menu_popup_attach(view_t* baseview)
{
  slp.view  = view_get_subview(baseview, "song_popup_list");
  slp.items = VNEW(); // REL 0

  slp.textstyle.font      = config_get("font_path");
  slp.textstyle.align     = TA_CENTER;
  slp.textstyle.margin    = 10;
  slp.textstyle.size      = 30.0;
  slp.textstyle.textcolor = 0x000000FF;
  slp.textstyle.backcolor = 0xF5F5F5FF;

  // create items

  VADDR(slp.items, ui_song_menu_popupitem_new(0, "Select song"));
  VADDR(slp.items, ui_song_menu_popupitem_new(1, "Select range"));
  VADDR(slp.items, ui_song_menu_popupitem_new(2, "Select all"));
  VADDR(slp.items, ui_song_menu_popupitem_new(3, "Jump to current song"));
  VADDR(slp.items, ui_song_menu_popupitem_new(4, "Edit selected songs"));
  VADDR(slp.items, ui_song_menu_popupitem_new(5, "Delete selected songs"));

  // add list handler to view

  vh_list_add(slp.view, ((vh_list_inset_t){0, 10, 0, 10}), ui_song_menu_popup_item_for_index, NULL, NULL);
}

void ui_song_menu_popup_detach()
{
  REL(slp.items);
}

void ui_song_menu_popup_on_item_delete(void* userdata, void* data)
{
  vec_t* selected = VNEW(); // REL 0

  ui_songlist_get_selected(selected);
  mem_describe(selected, 0);

  for (int index = 0; index < selected->length; index++)
  {
    map_t* entry = selected->data[index];
    lib_delete_file(config_get("lib_path"), entry);
    db_remove_entry(entry);
    visible_update();
    ui_songlist_update();
  }

  REL(selected); // REL 0
}

void ui_song_menu_popup_on_item_select(view_t* itemview, int index, vh_lcell_t* cell, ev_t ev)
{
  ui_popup_switcher_toggle("song_popup_page");

  if (index == 0) ui_songlist_select(index);
  if (index == 1) ui_songlist_select_range(index);
  if (index == 2) ui_songlist_select_all();
  if (index == 3) ui_play_jump_to();
  if (index == 4) ui_editor_popup_show();
  if (index == 5)
  {
    vec_t* selected = VNEW(); // REL 0
    ui_songlist_get_selected(selected);

    cb_t* acc_cb = cb_new(ui_song_menu_popup_on_item_delete, NULL);                                     // REL 1
    char* text   = cstr_new_format(100, "Are you sure you want to delete %i items?", selected->length); // REL 2

    ui_decision_popup_show(text, acc_cb, NULL);

    REL(text);     // REL 2
    REL(acc_cb);   // REL 1
    REL(selected); // REL 0
  }
}

view_t* ui_song_menu_popupitem_new(int index, char* text)
{
  char*   rowid    = cstr_new_format(100, "smlist_item%i", index); // REL 0
  char*   cellid   = cstr_new_format(100, "%s%s", rowid, "field"); // REL 1
  view_t* rowview  = view_new(rowid, (r2_t){0, 0, 460, 50});       // REL 2
  view_t* cellview = view_new(cellid, (r2_t){0, 0, 250, 50});      // REL 3

  vh_litem_add(rowview, NULL);
  vh_litem_set_on_select(rowview, ui_song_menu_popup_on_item_select);
  vh_litem_add_cell(rowview, "field", 240, cellview);
  vh_litem_upd_index(rowview, index);

  uint32_t color1 = (index % 2 == 0) ? 0xEFEFEFFF : 0xE5E5E5FF;

  slp.textstyle.backcolor = color1;

  tg_text_add(cellview);
  tg_text_set(cellview, text, slp.textstyle);

  REL(rowid);    // REL 0
  REL(cellid);   // REL 1
  REL(cellview); // REL 3

  return rowview;
}

view_t* ui_song_menu_popup_item_for_index(int index, void* userdata, view_t* listview, int* item_count)
{
  if (index < 0)
    return NULL; // no items before 0
  if (index >= slp.items->length)
    return NULL; // no more items

  *item_count = slp.items->length;

  return slp.items->data[index];
}

#endif

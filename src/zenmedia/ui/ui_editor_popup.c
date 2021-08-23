#ifndef ui_editor_popup_h
#define ui_editor_popup_h

#include "view.c"
#include "zc_map.c"

void ui_editor_popup_attach(view_t* view);
void ui_editor_popup_detach();
void ui_editor_popup_show();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "coder.c"
#include "column.c"
#include "config.c"
#include "database.c"
#include "text.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_alert_popup.c"
#include "ui_decision_popup.c"
#include "ui_filelist.c"
#include "ui_inputfield_popup.c"
#include "ui_manager.c"
#include "ui_popup_switcher.c"
#include "vh_button.c"
#include "vh_list.c"
#include "vh_list_head.c"
#include "vh_list_item.c"
#include "vh_textinput.c"
#include "vh_textinput_scroller.c"
#include "zc_callback.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_graphics.c"
#include "zc_vector.c"
#include <string.h>

void    ui_editor_popup_set_songs(vec_t* vec);
void    ui_editor_popup_create_table();
void    ui_editor_popup_on_accept();
view_t* ui_editor_popup_item_for_index(int index, void* userdata, view_t* listview, int* item_count);
void    ui_editor_popup_on_button_down(void* userdata, void* data);
void    ui_editor_popup_on_header_field_select(view_t* view, char* id, ev_t ev);
void    ui_editor_popup_on_header_field_insert(view_t* view, int src, int tgt);
void    ui_editor_popup_on_header_field_resize(view_t* view, char* id, int size);
void    ui_editor_popup_select_item(view_t* itemview, int index, vh_lcell_t* cell, ev_t ev);
void    ui_editor_popup_input_cell_value_changed(view_t* inputview);
void    ui_editor_popup_input_cell_edit_finished(view_t* inputview);

view_t* ui_editor_popup_editorcell_item_for_index(int index, void* userdata, view_t* listview, int* item_count);
void    ui_editor_popup_on_text(view_t* view, void* userdata);

struct _ui_editor_popup_t
{
  int song_count; // song count for final message

  view_t* list_view;  // editor table
  view_t* head_view;  // header veiw
  view_t* cover_view; // cover view

  view_t* textinput; // text editing cell that will be attached to edited row
  view_t* textvalue; // actual value cell that will be rattached after edit

  map_t* attributes; // current attributes in table

  vec_t* fields;
  vec_t* items;
  vec_t* cols;

  map_t* changed;
  vec_t* removed;
  char*  cover;

  char*    editor_key; // actually edited key
  uint32_t editor_col; // editor cell color

  view_t*     sel_item;
  textstyle_t textstyle;

} ep = {0};

char* mand_fields[] = {"artist", "album", "title", "date", "track", "disc", "genre", "tags", "comments", "encoder", "publisher", "copyright"};

void ui_editor_popup_attach(view_t* view)
{
  view_t* head_view  = view_get_subview(view, "song_editor_header");
  view_t* list_view  = view_get_subview(view, "editorlist");
  view_t* cover_view = view_get_subview(view, "coverview");
  view_t* acceptbtn  = view_get_subview(view, "editor_popup_accept_btn");
  view_t* rejectbtn  = view_get_subview(view, "editor_popup_reject_btn");
  view_t* uploadbtn  = view_get_subview(view, "uploadbtn");
  view_t* textinput  = view_get_subview(view, "editor_textinput_scroller");

  textstyle_t ts = {0};
  ts.font        = config_get("font_path");
  ts.margin      = 10.0;
  ts.align       = TA_LEFT;
  ts.size        = 30.0;
  ts.textcolor   = 0x000000FF;
  ts.backcolor   = 0;

  ep.head_view  = head_view;
  ep.list_view  = list_view;
  ep.cover_view = cover_view;

  ep.textinput  = textinput;
  ep.fields     = VNEW(); // REL 0
  ep.textstyle  = ts;
  ep.items      = VNEW(); // REL 1
  ep.attributes = MNEW(); // REL 2
  ep.changed    = MNEW(); // REL 3
  ep.removed    = VNEW(); // REL 4
  ep.cover      = NULL;   // REL 5
  ep.cols       = VNEW(); // REL 6

  r2_t frame = textinput->frame.local;
  frame.x    = 0;
  frame.y    = 0;

  view_set_frame(textinput, frame);
  view_remove_from_parent(textinput);

  ts.autosize = AS_AUTO;

  vh_textinput_scroller_add(textinput, "editor cell", "", ts, NULL);
  vh_textinput_set_on_deactivate(vh_textinput_scroller_get_input_view(textinput), ui_editor_popup_input_cell_edit_finished);

  ts.autosize = AS_FIX;

  ui_editor_popup_create_table();

  tg_text_add(uploadbtn);
  tg_text_set(uploadbtn, "add new image", ts);

  // tg_text_add(newfieldbtn);
  // tg_text_set(newfieldbtn, "add new field", ts);

  cb_t* but_cb = cb_new(ui_editor_popup_on_button_down, NULL); // REL 7

  vh_button_add(acceptbtn, VH_BUTTON_NORMAL, but_cb);
  vh_button_add(rejectbtn, VH_BUTTON_NORMAL, but_cb);
  vh_button_add(uploadbtn, VH_BUTTON_NORMAL, but_cb);
  // vh_button_add(newfieldbtn, VH_BUTTON_NORMAL, but_cb);

  tg_text_add(head_view);
  tg_text_set(head_view, "Editing 1 data", ts);

  // force texture initialization on cover view
  cover_view->tex_gen(cover_view);

  REL(but_cb); // REL 7
}

void ui_editor_popup_detach()
{
  REL(ep.fields);
  REL(ep.items);
  REL(ep.attributes);
  REL(ep.changed);
  REL(ep.removed);
  if (ep.cover) REL(ep.cover);
  REL(ep.cols);
}

void ui_editor_popup_show()
{
  vec_t* selected = VNEW(); // REL 0
  ui_filelist_get_selected(selected);

  if (selected->length > 0)
  {
    ui_editor_popup_set_songs(selected);
    ui_popup_switcher_toggle("song_editor_popup_page");
  }
  else
    ui_alert_popup_show("No songs are selected.");

  REL(selected); // REL 0
}

void ui_editor_popup_on_accept_cover(void* userdata, void* data)
{
  char* path_cs    = str_new_cstring(data);                                    // REL 0
  char* path_final = cstr_new_path_normalize(path_cs, config_get("wrk_path")); // REL 1
  // check if image is valid
  bm_t* image = coder_load_image(path_final);

  printf("PATH %s\n", path_final);

  if (image)
  {
    if (ep.cover) REL(ep.cover);
    ep.cover = RET(path_final);
    coder_load_image_into(path_final, ep.cover_view->texture.bitmap);
    REL(image);
  }
  else
  {
    ui_alert_popup_show("Not a valid image file.");
  }
  REL(path_cs);    // REL 0
  REL(path_final); // REL 1
}

void ui_editor_popup_on_button_down(void* userdata, void* data)
{
  view_t* view = data;
  if (strcmp(view->id, "editor_popup_accept_btn") == 0)
  {
    char* message;
    if (ep.cover)
      message = cstr_new_format(150, "%i fields will be changed cover will be %s on %i items, are you sure you want these modifications?",
                                ep.changed->count,
                                ep.cover,
                                ep.song_count); // REL 0
    else
      message = cstr_new_format(150, "%i fields will be changed on %i items, are you sure you want these modifications?",
                                ep.changed->count,
                                ep.song_count); // REL 0

    cb_t* acc_cb = cb_new(ui_editor_popup_on_accept, NULL); // REL 1
    ui_decision_popup_show(message, acc_cb, NULL);
    REL(message); // REL 0
    REL(acc_cb);  // REL 1
  }
  if (strcmp(view->id, "editor_popup_reject_btn") == 0)
  {
    ui_popup_switcher_toggle("song_editor_popup_page");
  }
  if (strcmp(view->id, "uploadbtn") == 0)
  {
    cb_t* acc_cb = cb_new(ui_editor_popup_on_accept_cover, NULL); // REL 2
    ui_inputfield_popup_show("Path to cover art image :", acc_cb, NULL);
    REL(acc_cb); // REL 2
  }
}

void ui_editor_popup_create_table()
{
  VADDR(ep.cols, col_new("key", 140, 0));
  VADDR(ep.cols, col_new("value", 380, 1));
  // VADDR(ep.cols, col_new("delete", 80, 2));

  // create header

  view_t* header = view_new("songeditorlist_header", (r2_t){0, 0, 10, 30}); // REL 0
  tg_css_add(header);

  vh_lhead_add(header);
  vh_lhead_set_on_select(header, ui_editor_popup_on_header_field_select);
  vh_lhead_set_on_insert(header, ui_editor_popup_on_header_field_insert);
  vh_lhead_set_on_resize(header, ui_editor_popup_on_header_field_resize);

  ep.textstyle.align     = TA_LEFT;
  ep.textstyle.backcolor = 0xFFFFFFFF;

  for (int i = 0; i < ep.cols->length; i++)
  {
    col_t*  col     = ep.cols->data[i];
    char*   id      = cstr_new_format(100, "%s%s", header->id, col->id); // REL 1
    view_t* colview = view_new(id, (r2_t){0, 0, col->size, 30});         // REL 2

    tg_text_add(colview);
    tg_text_set(colview, col->id, ep.textstyle);
    vh_lhead_add_cell(header, col->id, col->size, colview);

    REL(id);      // REL 1
    REL(colview); // REL 2
  }

  vh_list_add(ep.list_view, ((vh_list_inset_t){30, 10, 0, 10}), ui_editor_popup_item_for_index, NULL, NULL);
  vh_list_set_header(ep.list_view, header);

  REL(header);
}

// header related

void ui_editor_popup_on_header_field_select(view_t* view, char* id, ev_t ev)
{
  // (*uisp.on_header_select)(id);
}

void ui_editor_popup_on_header_field_insert(view_t* view, int src, int tgt)
{
  // update in fields so new items will use updated order
  col_t* col = ep.cols->data[src];

  RET(col);
  VREM(ep.cols, col);
  vec_ins(ep.cols, col, tgt);
  REL(col);

  // update all items and cache
  vec_t* items = vh_list_items(ep.list_view);
  for (int i = 0; i < items->length; i++)
  {
    view_t* item = items->data[i];
    vh_litem_swp_cell(item, src, tgt);
  }
}

void ui_editor_popup_on_header_field_resize(view_t* view, char* id, int size)
{
  // update in fields so new items will use updated size
  for (int i = 0; i < ep.cols->length; i++)
  {
    col_t* col = ep.cols->data[i];
    if (strcmp(col->id, id) == 0)
    {
      col->size = size;
      break;
    }
  }

  // update all items and cache
  vec_t* items = vh_list_items(ep.list_view);
  for (int i = 0; i < items->length; i++)
  {
    view_t* item = items->data[i];
    vh_litem_upd_cell_size(item, id, size);
  }
}

// row related

view_t* ui_editor_popup_item_for_index(int index, void* userdata, view_t* list_view, int* item_count)
{
  if (index < 0) return NULL;                 // no items before 0
  if (index >= ep.items->length) return NULL; // no more items

  *item_count = ep.items->length;

  return ep.items->data[index];
}

// cell related

view_t* ui_editor_popup_new_item()
{
  static int itemcnt;

  char*   id      = cstr_new_format(100, "editor_popup_item%i", itemcnt++); // REL 0
  view_t* rowview = view_new(id, (r2_t){0, 0, 0, 35});                      // REL 1

  vh_litem_add(rowview, NULL);
  vh_litem_set_on_select(rowview, ui_editor_popup_select_item);

  for (int i = 0; i < ep.cols->length; i++)
  {
    col_t*  col     = ep.cols->data[i];
    char*   id1     = cstr_new_format(100, "%s%s", rowview->id, col->id); // REL 2
    view_t* colview = view_new(id1, (r2_t){0, 0, col->size, 35});         // REL 3
    REL(id1);                                                             // REL 2

    tg_text_add(colview);
    tg_text_set(colview, col->id, ep.textstyle);

    vh_litem_add_cell(rowview, col->id, col->size, colview);

    REL(colview); // REL 3
  }

  REL(id); // REL 0

  return rowview;
}

void ui_editor_popup_select_item(view_t* itemview, int index, vh_lcell_t* cell, ev_t ev)
{
  char* key   = ep.fields->data[index];
  char* value = MGET(ep.attributes, key);

  if (key[0] != 'f')
  {

    if (strcmp(cell->id, "value") == 0)
    {
      uint32_t color1 = (index % 2 == 0) ? 0xFEFEFEFF : 0xEFEFEFFF;

      /* ep.textstyle.backcolor = color1; */
      ep.sel_item   = itemview;
      ep.editor_col = color1;

      /* char*   id        = cstr_new_format(100, "%s%s%s", itemview->id, key, "edit"); // REL 0 */
      /* view_t* inputcell = view_new(id, cell->view->frame.local); */

      ep.textstyle.backcolor = color1;
      vh_textinput_set_text(vh_textinput_scroller_get_input_view(ep.textinput), value);
      /* vh_textinput_set_on_deactivate(inputcell, ui_editor_popup_input_cell_edit_finished); // listen for text editing finish */

      ep.editor_key = key;

      ep.textvalue = vh_litem_get_cell(itemview, "value");

      vh_textinput_scroller_activate(ep.textinput, 1); // activate text input

      vh_list_lock_scroll(ep.list_view, 1);               // lock scrolling of list to avoid going out screen
      vh_litem_rpl_cell(itemview, "value", ep.textinput); // replacing simple text cell with input cell

      /* REL(id); */
    }

    if (strcmp(cell->id, "delete") == 0)
    {
      ep.textstyle.backcolor = 0xFF0000AA;
      tg_text_set(vh_litem_get_cell(itemview, "key"), key, ep.textstyle);
      tg_text_set(vh_litem_get_cell(itemview, "value"), value, ep.textstyle);
      //tg_text_set(vh_litem_get_cell(itemview, "delete"), "Remove", ep.textstyle);

      VADD(ep.removed, key);
    }
  }
}

void ui_editor_popup_input_cell_edit_finished(view_t* inputview)
{
  str_t* str   = vh_textinput_scroller_get_text(ep.textinput);
  char*  text  = str_new_cstring(str); // REL 0
  char*  value = MGET(ep.attributes, ep.editor_key);

  if (value == NULL) value = "";

  if (strcmp(value, text) != 0)
  {
    MPUT(ep.changed, ep.editor_key, text);

    if (strlen(text) > 0)
      ep.editor_col = 0xAAFFAAFF;
    else
      ep.editor_col = 0xFFAAAAFF;
  }

  // replace textinput cell with simple text cell

  //char*   id       = cstr_new_format(100, "%s%s", ep.sel_item->id, "value"); // REL 1
  //view_t* textcell = view_new(id, inputview->frame.local);                   // REL 2

  ep.textstyle.backcolor = ep.editor_col;

  //tg_text_add(textcell);
  tg_text_set(ep.textvalue, text, ep.textstyle);

  vh_list_lock_scroll(ep.list_view, 0);
  vh_litem_rpl_cell(ep.sel_item, "value", ep.textvalue);

  // textcell->blocks_touch = 0;

  // REL(id);       // REL 1
  // REL(textcell); // REL 2
  REL(text); // REL 0

  vh_textinput_set_text(vh_textinput_scroller_get_input_view(ep.textinput), "");
}

int ui_editor_popup_comp_text(void* left, void* right)
{
  char* la = left;
  char* ra = right;

  return strcmp(la, ra);
}

void ui_editor_popup_remove_str(vec_t* vec, char* str)
{
  for (int index = 0; index < vec->length; index++)
  {
    char* val = (char*)vec->data[index];
    if (strcmp(val, str) == 0)
    {
      vec_rem_at_index(vec, index);
      return;
    }
  }
}

void ui_editor_popup_create_items()
{
  // reset temporary fields containers
  vec_reset(ep.fields);
  // vec_reset(ep.items);

  // reset list handler
  vh_list_reset(ep.list_view);

  // add mandatory fields and remaining fields

  // store song and extract fields
  vec_t* tmpfields = VNEW(); // REL 0
  map_t* tmpmap    = MNEW(); // REL 1

  map_keys(ep.attributes, tmpfields);

  for (int index = 0; index < tmpfields->length; index++)
  {
    char* key = tmpfields->data[index];
    char* val = MGET(ep.attributes, key);
    MPUT(tmpmap, key, val);
  }

  for (int index = 0; index < sizeof(mand_fields) / sizeof(mand_fields[0]); index++)
  {
    char* field = cstr_new_format(100, "meta/%s", mand_fields[index]); // REL 3
    map_del(tmpmap, field);
    VADD(ep.fields, field);
    REL(field); // REL 3
  }

  vec_reset(tmpfields);
  map_keys(tmpmap, tmpfields);
  vec_sort(tmpfields, VSD_DSC, ui_editor_popup_comp_text);

  // add remaining fields in tmpfields
  vec_add_in_vector(ep.fields, tmpfields);

  REL(tmpfields); // REL 0
  REL(tmpmap);    // REL 1

  // create items

  for (int index = 0; index < ep.fields->length; index++)
  {
    view_t* item;
    if (index < ep.items->length)
    {
      item = ep.items->data[index];
    }
    else
    {
      item = ui_editor_popup_new_item(); // REL 4
      VADD(ep.items, item);
      REL(item);
    }

    vh_litem_upd_index(item, index);

    char* key   = ep.fields->data[index];
    char* value = MGET(ep.attributes, key);

    if (value == NULL) value = "";

    uint32_t color1 = (index % 2 == 0) ? 0xFEFEFEFF : 0xEFEFEFFF;
    uint32_t color2 = (index % 2 == 0) ? 0xF8F8F8FF : 0xE8E8E8FF;

    // "file" fields are non-editable
    if (key[0] == 'f') color1 &= 0xFFDDDDFF;
    if (key[0] == 'f') color2 &= 0xFFDDDDFF;

    ep.textstyle.backcolor = color1;
    tg_text_set(vh_litem_get_cell(item, "key"), key + 5, ep.textstyle); // show last key name component
    ep.textstyle.backcolor = color2;
    tg_text_set(vh_litem_get_cell(item, "value"), value, ep.textstyle);

    // if (key[0] != 'f') tg_text_set(vh_litem_get_cell(item, "delete"), "Delete", ep.textstyle);
  }
}

void ui_editor_popup_set_songs(vec_t* vec)
{
  if (vec->length > 0)
  {
    map_reset(ep.attributes);
    map_reset(ep.changed);
    vec_reset(ep.removed);

    vec_t* fields = VNEW(); // REL 0
    vec_t* values = VNEW(); // REL 1

    for (int index = 0; index < vec->length; index++)
    {
      map_t* song = vec->data[index];
      vec_reset(fields);
      vec_reset(values);
      map_keys(song, fields);

      for (int fi = 0; fi < fields->length; fi++)
      {
        char* field = fields->data[fi];
        char* value = MGET(song, field);
        char* curr  = MGET(ep.attributes, field);

        if (curr == NULL)
        {
          MPUT(ep.attributes, field, value);
        }
        else
        {
          if (strcmp(curr, value) != 0) MPUTR(ep.attributes, field, cstr_new_cstring("MULTIPLE"));
        }
      }
    }
    ui_editor_popup_create_items();

    REL(fields); // REL 0
    REL(values); // REL 1
  }

  ep.song_count          = vec->length;
  ep.textstyle.backcolor = 0;

  char* text = cstr_new_format(100, "Editing %i song(s)", vec->length); // REL 2
  tg_text_set(ep.head_view, text, ep.textstyle);
  REL(text); // REL 2

  // reset cover

  gfx_rect(ep.cover_view->texture.bitmap, 0, 0, ep.cover_view->texture.bitmap->w, ep.cover_view->texture.bitmap->h, 0, 1);

  if (vec->length == 1)
  {
    map_t* song = vec->data[0];
    char*  path = MGET(song, "file/path");
    char*  file = cstr_new_path_append(config_get("lib_path"), path); // REL 3

    coder_load_cover_into(file, ep.cover_view->texture.bitmap);

    ep.cover_view->texture.changed = 1;

    REL(file); // REL 3
  }
}

void ui_editor_popup_on_accept()
{
  ui_popup_switcher_toggle("song_editor_popup_page");

  vec_t* songs = VNEW(); // REL 0
  ui_filelist_get_selected(songs);

  /* /\* printf("SELECTED\n"); *\/ */
  /* /\* mem_describe(selected, 0); *\/ */
  /* printf("CHANGED\n"); */
  /* mem_describe(ep.changed, 0); */
  /* printf("REMOVED\n"); */
  /* mem_describe(ep.removed, 0); */

  char* libpath = config_get("lib_path");

  // update metadata in media files

  for (int index = 0; index < songs->length; index++)
  {
    map_t* song = songs->data[index];
    char*  path = MGET(song, "file/path");

    int result = coder_write_metadata(libpath, path, ep.cover, ep.changed, ep.removed);
    if (result >= 0)
    {
      // modify song in db also if metadata is successfully written into file
      db_update_metadata(path, ep.changed, ep.removed);
    }
  }

  db_write(libpath);

  REL(songs); // REL 0

  ui_filelist_refresh();
}

#endif

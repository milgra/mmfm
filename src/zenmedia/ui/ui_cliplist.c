#ifndef ui_cliplist_h
#define ui_cliplist_h

#include "view.c"
#include "zc_map.c"

void ui_cliplist_select(int index);
void ui_cliplist_select_all();
void ui_cliplist_select_range(int index);
void ui_cliplist_get_selected(vec_t* vec);

void ui_cliplist_attach(view_t* base);
void ui_cliplist_detach();

void ui_cliplist_update();
void ui_cliplist_refresh();
void ui_cliplist_toggle_pause(int state);
void ui_cliplist_select_and_show(int index);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "column.c"
#include "config.c"
#include "selection.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_play_controls.c"
#include "ui_popup_switcher.c"
#include "vh_button.c"
#include "vh_list.c"
#include "vh_list_head.c"
#include "vh_list_item.c"
#include "visible.c"

void on_cliplist_header_field_select(view_t* view, char* id, ev_t ev);
void on_cliplist_header_field_insert(view_t* view, int src, int tgt);
void on_cliplist_header_field_resize(view_t* view, char* id, int size);

view_t* ui_cliplist_item_for_index(int index, void* userdata, view_t* listview, int* item_count);
void    ui_cliplist_item_recycle(view_t* item, void* userdata, view_t* listview);

struct ui_cliplist_t
{
  view_t*     view;    // table view
  vec_t*      items;   // all items
  vec_t*      cache;   // item cache
  vec_t*      columns; // fileds in table
  vec_t*      fields;  // real field names
  textstyle_t textstyle;

  uint32_t color_s; // color selected
  int32_t  index_s; // index selected

  void (*on_select)(int index);
} cl = {0};

void ui_cliplist_attach(view_t* base)
{
  assert(base != NULL);

  cl.view    = view_get_subview(base, "cliplist");
  cl.items   = VNEW(); // REL -1
  cl.cache   = VNEW(); // REL 0
  cl.columns = VNEW(); // REL 1
  cl.fields  = VNEW(); // REL 2

  cl.color_s = 0x55FF55FF;

  cl.textstyle.font        = config_get("font_path");
  cl.textstyle.margin_left = 10;
  cl.textstyle.size        = 30.0;
  cl.textstyle.textcolor   = 0x000000FF;
  cl.textstyle.backcolor   = 0xF5F5F5FF;

  // create columns

  /*   MPUTR(file, "type", cstr_new_format(5, "%s", (tflag == FTW_D) ? "d" : (tflag == FTW_DNR) ? "dnr" : (tflag == FTW_DP) ? "dp" : (tflag == FTW_F) ? "f" : (tflag == FTW_NS) ? "ns" : (tflag == FTW_SL) ? "sl" : (tflag == FTW_SLN) ? "sln" : "???")); */
  /* MPUTR(file, "path", cstr_new_format(PATH_MAX + NAME_MAX, "%s", fpath)); */
  /* MPUTR(file, "basename", cstr_new_format(NAME_MAX, "%s", fpath + ftwbuf->base)); */
  /* MPUTR(file, "level", cstr_new_format(20, "%li", ftwbuf->level)); */
  /* MPUTR(file, "device", cstr_new_format(20, "%li", sb->st_dev)); */
  /* MPUTR(file, "size", cstr_new_format(20, "%li", sb->st_size)); */
  /* MPUTR(file, "inode", cstr_new_format(20, "%li", sb->st_ino)); */
  /* MPUTR(file, "links", cstr_new_format(20, "%li", sb->st_nlink)); */
  /* MPUTR(file, "userid", cstr_new_format(20, "%li", sb->st_uid)); */
  /* MPUTR(file, "groupid", cstr_new_format(20, "%li", sb->st_gid)); */
  /* MPUTR(file, "deviceid", cstr_new_format(20, "%li", sb->st_rdev)); */
  /* MPUTR(file, "blocksize", cstr_new_format(20, "%li", sb->st_blksize)); */
  /* MPUTR(file, "blocks", cstr_new_format(20, "%li", sb->st_blocks)); */
  /* MPUTR(file, "last_access", cstr_new_format(20, "%li", sb->st_atim)); */
  /* MPUTR(file, "last_modification", cstr_new_format(20, "%li", sb->st_mtim)); */
  /* MPUTR(file, "last_status", cstr_new_format(20, "%li", sb->st_ctim)); */

  VADDR(cl.columns, col_new("ind", 60, 0));
  VADDR(cl.columns, col_new("basename", 200, 1));
  VADDR(cl.columns, col_new("size", 200, 2));
  VADDR(cl.columns, col_new("type", 200, 2));
  VADDR(cl.columns, col_new("userid", 350, 3));
  VADDR(cl.columns, col_new("username", 70, 4));
  VADDR(cl.columns, col_new("groupid", 150, 5));
  VADDR(cl.columns, col_new("groupname", 60, 6));
  VADDR(cl.columns, col_new("last_access", 60, 7));
  VADDR(cl.columns, col_new("last_modification", 50, 8));
  VADDR(cl.columns, col_new("last_status", 40, 9));
  VADDR(cl.columns, col_new("level", 100, 10));
  VADDR(cl.columns, col_new("device", 80, 11));
  VADDR(cl.columns, col_new("inode", 55, 12));
  VADDR(cl.columns, col_new("blocksize", 55, 13));
  VADDR(cl.columns, col_new("blocks", 150, 14));
  VADDR(cl.columns, col_new("links", 155, 15));

  VADDR(cl.fields, cstr_new_cstring("index"));
  VADDR(cl.fields, cstr_new_cstring("basename"));
  VADDR(cl.fields, cstr_new_cstring("size"));
  VADDR(cl.fields, cstr_new_cstring("type"));
  VADDR(cl.fields, cstr_new_cstring("userid"));
  VADDR(cl.fields, cstr_new_cstring("username"));
  VADDR(cl.fields, cstr_new_cstring("groupid"));
  VADDR(cl.fields, cstr_new_cstring("groupname"));
  VADDR(cl.fields, cstr_new_cstring("last_access"));
  VADDR(cl.fields, cstr_new_cstring("last_modification"));
  VADDR(cl.fields, cstr_new_cstring("last_status"));
  VADDR(cl.fields, cstr_new_cstring("level"));
  VADDR(cl.fields, cstr_new_cstring("device"));
  VADDR(cl.fields, cstr_new_cstring("inode"));
  VADDR(cl.fields, cstr_new_cstring("blocksize"));
  VADDR(cl.fields, cstr_new_cstring("blocks"));
  VADDR(cl.fields, cstr_new_cstring("links"));

  // add header handler

  view_t* header                  = view_new("cliplist_header", (r2_t){0, 0, 10, 30}); // REL 3
  header->layout.background_color = 0x33333355;
  /* header->layout.shadow_blur      = 3; */
  /* header->layout.border_radius    = 3; */
  tg_css_add(header);

  vh_lhead_add(header);
  vh_lhead_set_on_select(header, on_cliplist_header_field_select);
  vh_lhead_set_on_insert(header, on_cliplist_header_field_insert);
  vh_lhead_set_on_resize(header, on_cliplist_header_field_resize);

  for (int i = 0; i < cl.columns->length; i++)
  {
    col_t*  cell     = cl.columns->data[i];
    char*   id       = cstr_new_format(100, "%s%s", header->id, cell->id); // REL 4
    char*   dragid   = cstr_new_format(100, "%sdrag", id);
    view_t* cellview = view_new(id, (r2_t){0, 0, cell->size, 30});          // REL 5
    view_t* dragview = view_new(dragid, (r2_t){cell->size - 5, 10, 5, 10}); // REL 6

    tg_text_add(cellview);
    tg_text_set(cellview, cell->id, cl.textstyle);

    dragview->layout.background_color = 0x00000022;
    dragview->layout.right            = 5;
    tg_css_add(dragview);
    view_add_subview(cellview, dragview);
    dragview->blocks_touch = 0;

    vh_lhead_add_cell(header, cell->id, cell->size, cellview);

    REL(id); // REL 4
    REL(dragid);
    REL(cellview); // REL 5
    REL(dragview); // REL 6
  }

  // add list handler to view

  vh_list_add(cl.view,
              ((vh_list_inset_t){30, 200, 0, 10}),
              ui_cliplist_item_for_index,
              ui_cliplist_item_recycle,
              NULL);
  vh_list_set_header(cl.view, header);

  // select first song by default

  selection_add(0);

  REL(header); // REL 3
}

void ui_cliplist_detach()
{
  vh_list_reset(cl.view);

  REL(cl.items);   // REL -1
  REL(cl.cache);   // REL 0
  REL(cl.columns); // REL 1
  REL(cl.fields);  // REL 2
}

void ui_cliplist_update()
{
  selection_res();
  vh_list_reset(cl.view);
}

void ui_cliplist_refresh()
{
  vh_list_refresh(cl.view);
}

void ui_cliplist_toggle_pause(int state)
{
  /* if (state) */
  /*   cl.color_s = 0xFF5555FF; */
  /* else */
  /*   cl.color_s = 0x55FF55FF; */

  // view_t* item = vh_list_item_for_index(cl.view, cl.index_s);

  // if (item) clipitem_update_row(item, cl.index_s, cl.songs->data[cl.index_s], cl.color_s);
}

// header

void on_cliplist_header_field_select(view_t* view, char* id, ev_t ev)
{
  for (int index = 0; index < cl.columns->length; index++)
  {
    col_t* cell = cl.columns->data[index];
    if (strcmp(cell->id, id) == 0)
    {
      char* field = cl.fields->data[index];
      visible_set_sortfield(field, 1);
      ui_cliplist_refresh();
    }
  }
}

void on_cliplist_header_field_insert(view_t* view, int src, int tgt)
{
  // update in real fields to sync columns state
  char* field = cl.fields->data[src];

  RET(field);
  VREM(cl.fields, field);
  vec_ins(cl.fields, field, tgt);
  REL(field);

  // update in columns so new items will use updated order
  col_t* cell = cl.columns->data[src];

  RET(cell);
  VREM(cl.columns, cell);
  vec_ins(cl.columns, cell, tgt);
  REL(cell);

  // update all items and cache
  vec_t* items = vh_list_items(cl.view);
  for (int index = 0; index < items->length; index++)
  {
    view_t* item = items->data[index];
    vh_litem_swp_cell(item, src, tgt);
  }

  items = cl.cache;
  for (int index = 0; index < items->length; index++)
  {
    view_t* item = items->data[index];
    vh_litem_swp_cell(item, src, tgt);
  }
}

void on_cliplist_header_field_resize(view_t* view, char* id, int size)
{
  // update in columns so new items will use updated size
  for (int i = 0; i < cl.columns->length; i++)
  {
    col_t* cell = cl.columns->data[i];
    if (strcmp(cell->id, id) == 0)
    {
      cell->size = size;
      break;
    }
  }

  // update all items and cache
  vec_t* items = vh_list_items(cl.view);

  for (int index = 0; index < items->length; index++)
  {
    view_t* item = items->data[index];
    vh_litem_upd_cell_size(item, id, size);
  }

  items = cl.cache;
  for (int index = 0; index < items->length; index++)
  {
    view_t* item = items->data[index];
    vh_litem_upd_cell_size(item, id, size);
  }
}

void ui_cliplist_select(int index)
{
  // selection_res();
  selection_add(cl.index_s);
  vh_list_refresh(cl.view);
}

void ui_cliplist_select_range(int index)
{
  selection_rng(cl.index_s);
  vh_list_refresh(cl.view);
}

void ui_cliplist_select_all()
{
  selection_res();
  selection_add(0);
  selection_rng(visible_song_count());
  vh_list_refresh(cl.view);
}

void ui_cliplist_get_selected(vec_t* vec)
{
  // if selection is empty, select current index
  if (selection_cnt() > 0)
  {
    selection_add_selection(visible_get_songs(), vec);
  }
}

void ui_cliplist_select_and_show(int index)
{
  selection_res();
  selection_add(index);
  vh_list_scroll_to_index(cl.view, index);
}

// items

void ui_cliplist_on_item_select(view_t* itemview, int index, vh_lcell_t* cell, ev_t ev)
{
  cl.index_s = index;
  if (ev.button == 1)
  {
    if (!ev.ctrl_down && !ev.shift_down) selection_res();

    if (ev.shift_down)
    {
      selection_rng(index);
    }
    else
    {
      selection_add(index);
    }

    if (ev.dclick)
    {
      ui_play_index(index);
    }

    vh_list_refresh(cl.view);
  }
  else if (ev.button == 3)
  {

    ui_popup_switcher_toggle("song_popup_page");

    /* if (selection_cnt() < 2) */
    /* { */
    /*   selection_res(); */
    /*   selection_add(index); */
    /*   vh_list_refresh(cl.view); */
    /* } */
  }
}

view_t* clipitem_new()
{
  static int item_cnt      = 0;
  char       idbuffer[100] = {0};
  snprintf(idbuffer, 100, "cliplist_item%i", item_cnt++);

  view_t* rowview = view_new(idbuffer, (r2_t){0, 0, 0, 35}); // REL 0

  vh_litem_add(rowview, NULL);
  vh_litem_set_on_select(rowview, ui_cliplist_on_item_select);

  for (int i = 0; i < cl.columns->length; i++)
  {
    col_t*  cell     = cl.columns->data[i];
    char*   id       = cstr_new_format(100, "%s%s", rowview->id, cell->id); // REL 0
    view_t* cellview = view_new(id, (r2_t){0, 0, cell->size, 35});          // REL 1
    tg_text_add(cellview);

    vh_litem_add_cell(rowview, cell->id, cell->size, cellview);

    REL(id);       // REL 0
    REL(cellview); // REL 1
  }

  return rowview;
}

void clipitem_update_row(view_t* rowview, int index, map_t* file, uint32_t color)
{
  char indbuffer[6];
  if (index > -1)
    snprintf(indbuffer, 6, "%i.", index);
  else
    snprintf(indbuffer, 6, "No.");

  cl.textstyle.textcolor = 0x000000FF;
  cl.textstyle.backcolor = color;

  vh_litem_upd_index(rowview, index);

  for (int index = 0; index < cl.columns->length; index++)
  {

    col_t* cell  = cl.columns->data[index];
    char*  field = cl.fields->data[index];

    if (MGET(file, field))
    {
      tg_text_set(vh_litem_get_cell(rowview, cell->id), MGET(file, field), cl.textstyle);
    }
    else
    {
      tg_text_set(vh_litem_get_cell(rowview, cell->id), "-", cl.textstyle);
    }
  }

  tg_text_set(vh_litem_get_cell(rowview, "ind"), indbuffer, cl.textstyle);
}

void ui_cliplist_item_recycle(view_t* item, void* userdata, view_t* listview)
{
  uint32_t index = vec_index_of_data(cl.cache, item);

  if (index == UINT32_MAX)
  {
    VADD(cl.cache, item);
  }
  else
    printf("item exists in cl.cache %s\n", item->id);
}

view_t* ui_cliplist_item_for_index(int index, void* userdata, view_t* listview, int* item_count)
{
  if (index < 0) return NULL;                     // no items before 0
  if (index >= visible_song_count()) return NULL; // no more items

  *item_count = visible_song_count();

  view_t* item;
  if (cl.cache->length > 0)
    item = cl.cache->data[0];
  else
  {
    item = clipitem_new();
    VADD(cl.items, item);
    REL(item);
  }

  VREM(cl.cache, item);

  if (selection_has(index))
  {
    clipitem_update_row(item, index, visible_song_at_index(index), cl.color_s);
  }
  else
  {
    uint32_t color1 = (index % 2 == 0) ? 0xEFEFEFFF : 0xE5E5E5FF;
    uint32_t color2 = (index % 2 == 0) ? 0xE5E5E5FF : 0xEFEFEFFF;

    clipitem_update_row(item, index, visible_song_at_index(index), color1);
  }

  return item;
}

#endif

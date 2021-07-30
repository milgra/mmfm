#ifndef textlist_h
#define textlist_h

#include "text.c"
#include "view.c"

typedef struct _textlist_t
{
  vec_t*      items;
  vec_t*      cache;
  vec_t*      sources;
  view_t*     view;
  str_t*      string;
  textstyle_t textstyle;

  void (*on_select)(int index);
} textlist_t;

textlist_t* textlist_new(view_t* view, textstyle_t textstyle, void (*on_select)(int));
void        textlist_update(textlist_t* tl);
void        textlist_set_datasource(textlist_t* tl, vec_t* items);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "tg_text.c"
#include "vh_list.c"
#include "vh_list_item.c"

void    textlist_del(void* p);
view_t* textlist_item_for_index(int index, void* data, view_t* listview, int* item_count);
void    textlist_item_recycle(view_t* item, void* data, view_t* listview);

void textlist_desc(void* p, int level)
{
  printf("textlist");
}

textlist_t* textlist_new(view_t* view, textstyle_t textstyle, void (*on_select)(int))
{
  textlist_t* tl = CAL(sizeof(textlist_t), textlist_del, textlist_desc);

  tl->items     = VNEW();
  tl->cache     = VNEW();
  tl->view      = view;
  tl->string    = str_new();
  tl->textstyle = textstyle;
  tl->on_select = on_select;

  vh_list_add(view, ((vh_list_inset_t){0, 10, 0, 10}), textlist_item_for_index, textlist_item_recycle, tl);

  return tl;
}

void textlist_del(void* p)
{
  textlist_t* tl = p;

  REL(tl->items);
  REL(tl->cache);
  REL(tl->string);
  if (tl->sources) REL(tl->sources);
}

void textlist_update(textlist_t* tl)
{
  vh_list_refresh(tl->view);
}

void textlist_set_datasource(textlist_t* tl, vec_t* sources)
{
  if (tl->sources) REL(tl->sources);
  tl->sources = RET(sources);
}

void on_textitem_select(view_t* itemview, int index, vh_lcell_t* cell, ev_t ev)
{
  vh_litem_t* vh = itemview->handler_data;
  textlist_t* tl = vh->userdata;

  (*tl->on_select)(vh->index);
}

view_t* textlist_new_item(textlist_t* tl)
{
  static int item_cnt = 0;

  char* item_id = cstr_new_format(100, "textlist_item%i", item_cnt++); // REL 0
  char* cell_id = cstr_new_format(100, "%s%s", item_id, "cell");       // REL 1

  view_t* item_view = view_new(item_id, (r2_t){0, 0, 0, 35});                       // REL 2
  view_t* cell_view = view_new(cell_id, (r2_t){0, 0, tl->view->frame.local.w, 35}); // REL 3

  tg_text_add(cell_view);

  vh_litem_add(item_view, tl);
  vh_litem_set_on_select(item_view, on_textitem_select);
  vh_litem_add_cell(item_view, "cell", 200, cell_view);

  REL(item_id);   // REL 0
  REL(cell_id);   // REL 1
  REL(cell_view); // REL 3

  return item_view;
}

void textlist_item_recycle(view_t* item, void* data, view_t* listview)
{
  textlist_t* tl = data;

  uint32_t index = vec_index_of_data(tl->cache, item);

  if (index == UINT32_MAX)
  {
    VADD(tl->cache, item);
  }
  else
    printf("item exists in sl.cache %s\n", item->id);
}

view_t* textlist_item_for_index(int index, void* data, view_t* listview, int* item_count)
{
  textlist_t* tl = data;

  if (index < 0) return NULL;                    // no sources before 0
  if (index >= tl->sources->length) return NULL; // no more sources

  *item_count = tl->sources->length;

  view_t* item;
  if (tl->cache->length > 0)
    item = tl->cache->data[0];
  else
  {
    item = textlist_new_item(tl);
    VADD(tl->items, item);
    REL(item);
  }

  VREM(tl->cache, item);

  uint32_t color = (index % 2 == 0) ? 0xEFEFEFFF : 0xE5E5E5FF;

  tl->textstyle.backcolor = color;

  str_reset(tl->string);
  str_add_bytearray(tl->string, tl->sources->data[index]);
  int nw;
  int nh;
  text_measure(tl->string, tl->textstyle, item->frame.local.w, item->frame.local.h, &nw, &nh);

  if (nh < 35) nh = 35;

  view_t* cell = vh_litem_get_cell(item, "cell");

  r2_t frame = item->frame.local;
  frame.h    = nh;

  view_set_frame(item, frame);

  frame   = cell->frame.local;
  frame.h = nh;

  view_set_frame(cell, frame);

  vh_litem_upd_index(item, index);
  tg_text_set(cell, tl->sources->data[index], tl->textstyle);

  return item;
}

#endif

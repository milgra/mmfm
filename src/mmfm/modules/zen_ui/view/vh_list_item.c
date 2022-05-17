#ifndef vh_list_item2_h
#define vh_list_item2_h

#include "vh_list_cell.c"
#include "view.c"
#include "zc_cstring.c"
#include "zc_map.c"
#include "zc_vector.c"

typedef struct _vh_litem_t
{
  view_t* view;
  int     index;
  vec_t*  cells;
  void*   userdata;

  void (*on_select)(view_t* view, int index, vh_lcell_t* cell, ev_t ev);
} vh_litem_t;

void vh_litem_add(view_t* view, void* userdata);
void vh_litem_upd_index(view_t* view, int index);
void vh_litem_set_on_select(view_t* view, void (*on_select)(view_t*, int index, vh_lcell_t* cell, ev_t ev));

void    vh_litem_add_cell(view_t* view, char* id, int size, view_t* cellview);
view_t* vh_litem_get_cell(view_t* view, char* id);
void    vh_litem_rem_cell(char* id);
void    vh_litem_swp_cell(view_t* view, int src, int tgt);
void    vh_litem_rpl_cell(view_t* view, char* id, view_t* newcell);
void    vh_litem_upd_cell_size(view_t* view, char* id, int size);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "tg_css.c"

void vh_litem_evt(view_t* view, ev_t ev);
void vh_litem_del(void* p);

void vh_litem_desc(void* p, int level)
{
  printf("vh_litem");
}

void vh_litem_add(view_t* view, void* userdata)
{
  assert(view->handler == NULL && view->handler_data == NULL);

  vh_litem_t* vh = CAL(sizeof(vh_litem_t), vh_litem_del, vh_litem_desc);
  vh->cells      = VNEW();
  vh->userdata   = userdata;
  vh->view       = view;

  view->handler_data = vh;
  view->handler      = vh_litem_evt;
}

void vh_litem_del(void* p)
{
  vh_litem_t* list = p;
  REL(list->cells);
}

void vh_litem_upd_index(view_t* view, int index)
{
  vh_litem_t* vh = view->handler_data;
  vh->index      = index;
}

void vh_litem_set_on_select(view_t* view, void (*on_select)(view_t*, int index, vh_lcell_t* cell, ev_t ev))
{
  vh_litem_t* vh = view->handler_data;
  vh->on_select  = on_select;
}

void vh_litem_evt(view_t* view, ev_t ev)
{
  if (ev.type == EV_MUP)
  {
    vh_litem_t* vh = view->handler_data;

    // get selected cell
    for (int index = 0; index < vh->cells->length; index++)
    {
      vh_lcell_t* cell = vh->cells->data[index];
      if (ev.x > cell->view->frame.global.x && ev.x < cell->view->frame.global.x + cell->view->frame.global.w)
      {
        (*vh->on_select)(view, vh->index, cell, ev);
        break;
      }
    }
  }
}

void vh_litem_resize(view_t* view)
{
  vh_litem_t* vh = view->handler_data;

  vh_lcell_t* last   = vec_tail(vh->cells);
  r2_t        lframe = last->view->frame.local;
  r2_t        vframe = view->frame.local;
  vframe.w           = lframe.x + lframe.w;

  view_set_frame(view, vframe);
}

// cell handling

void vh_litem_add_cell(view_t* view, char* id, int size, view_t* cellview)
{
  vh_litem_t* vh   = view->handler_data;
  vh_lcell_t* cell = vh_lcell_new(id, size, cellview, vh->cells->length); // REL 0

  view_set_block_touch(cellview, 0, 1);

  // add subview
  view_add_subview(view, cellview);

  // store cell
  VADD(vh->cells, cell);

  // arrange and resize
  vh_lcell_arrange(vh->cells);
  vh_litem_resize(view);

  REL(cell);
}

view_t* vh_litem_get_cell(view_t* view, char* id)
{
  vh_litem_t* vh = view->handler_data;
  return vh_lcell_get(vh->cells, id);
}

void vh_litem_swp_cell(view_t* view, int src, int tgt)
{
  vh_litem_t* vh = view->handler_data;
  vh_lcell_ins(vh->cells, src, tgt);
}

void vh_litem_rpl_cell(view_t* view, char* id, view_t* newcell)
{
  vh_litem_t* vh = view->handler_data;

  for (int index = 0; index < vh->cells->length; index++)
  {
    vh_lcell_t* cell = vh->cells->data[index];
    if (strcmp(cell->id, id) == 0)
    {
      view_add_subview(view, newcell);
      view_remove_from_parent(cell->view);

      cell->view = newcell;
      break;
    }
  }
  vh_lcell_arrange(vh->cells);
  vh_litem_resize(view);
}

void vh_litem_upd_cell_size(view_t* view, char* id, int size)
{
  vh_litem_t* vh = view->handler_data;
  vh_lcell_set_size(vh->cells, id, size);
  vh_litem_resize(view);
}

#endif

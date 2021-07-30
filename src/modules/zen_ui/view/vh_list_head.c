// TODO it should re-use list_item parts

#ifndef vh_list_head_h
#define vh_list_head_h

#include "vh_list_cell.c"
#include "view.c"
#include "view_layout.c"
#include "zc_cstring.c"
#include "zc_map.c"
#include "zc_vector.c"

typedef struct _vh_lhead_t
{
  vec_t* cells;

  vh_lcell_t* dragged_cell;
  int         dragged_pos;
  int         dragged_flag;
  int         dragged_ind;
  vh_lcell_t* resized_cell;
  vh_lcell_t* resized_flag;

  void (*on_select)(view_t* view, char* id, ev_t ev);
  void (*on_insert)(view_t* view, int src, int tgt);
  void (*on_resize)(view_t* view, char* id, int width);
} vh_lhead_t;

void vh_lhead_add(view_t* view);

void vh_lhead_set_on_select(view_t* view, void (*on_select)(view_t*, char*, ev_t));
void vh_lhead_set_on_insert(view_t* view, void (*on_insert)(view_t*, int, int));
void vh_lhead_set_on_resize(view_t* view, void (*on_resize)(view_t*, char*, int));

view_t* vh_lhead_get_cell(view_t* view, char* id);
void    vh_lhead_add_cell(view_t* view, char* id, int size, view_t* cellview);
void    vh_lhead_rem_cell(char* id);
void    vh_lhead_swp_cell(char* ida, char* idb);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "tg_css.c"

void vh_lhead_evt(view_t* view, ev_t ev);
void vh_lhead_del(void* p);
void vh_lhead_resize(view_t* view);

void vh_lhead_desc(void* p, int level)
{
  printf("vh_lhead");
}

void vh_lhead_add(view_t* view)
{
  assert(view->handler == NULL && view->handler_data == NULL);

  vh_lhead_t* vh = CAL(sizeof(vh_lhead_t), vh_lhead_del, vh_lhead_desc);
  vh->cells      = VNEW();

  view->handler_data = vh;
  view->handler      = vh_lhead_evt;
}

void vh_lhead_del(void* p)
{
  vh_lhead_t* list = p;
  REL(list->cells);
}

void vh_lhead_set_on_select(view_t* view, void (*on_select)(view_t*, char*, ev_t))
{
  vh_lhead_t* vh = view->handler_data;
  vh->on_select  = on_select;
}

void vh_lhead_set_on_insert(view_t* view, void (*on_insert)(view_t*, int, int))
{
  vh_lhead_t* vh = view->handler_data;
  vh->on_insert  = on_insert;
}

void vh_lhead_set_on_resize(view_t* view, void (*on_resize)(view_t*, char*, int))
{
  vh_lhead_t* vh = view->handler_data;
  vh->on_resize  = on_resize;
}

void vh_lhead_evt(view_t* view, ev_t ev)
{
  if (ev.type == EV_MDOWN)
  {
    // check cell or cell border intersection
    vh_lhead_t* vh = view->handler_data;
    for (int i = 0; i < vh->cells->length; i++)
    {
      vh_lcell_t* cell = vh->cells->data[i];
      r2_t        f    = cell->view->frame.global;
      if (f.x < ev.x && f.x + f.w + 1 > ev.x)
      {
        if (ev.x > f.x + f.w - 10)
        {
          vh->resized_cell = cell;
        }
        else
        {
          vh->dragged_cell = cell;
          vh->dragged_pos  = ev.x - f.x;
          vh->dragged_ind  = i;
        }
        view_remove_from_parent(cell->view);
        view_add_subview(view, cell->view);
        break;
      }
    }
  }
  else if (ev.type == EV_MMOVE)
  {
    // resize or move cell
    if (ev.drag)
    {
      vh_lhead_t* vh = view->handler_data;

      if (vh->resized_cell)
      {
        r2_t fg = vh->resized_cell->view->frame.global;
        r2_t fl = vh->resized_cell->view->frame.local;
        fl.w    = ev.x - fg.x;
        view_set_frame(vh->resized_cell->view, fl);
        vh_lcell_arrange(vh->cells);
        vh_lhead_resize(view);
      }
      else if (vh->dragged_cell)
      {
        vh->dragged_flag = 1;
        r2_t fg          = vh->dragged_cell->view->frame.global;
        r2_t fl          = vh->dragged_cell->view->frame.local;
        // todo mouse event should contain local coordinates also
        fg.x = ev.x - vh->dragged_pos - view->frame.global.x;
        fg.y = 0;
        view_set_frame(vh->dragged_cell->view, fg);
      }
    }
  }
  else if (ev.type == EV_MUP)
  {
    vh_lhead_t* vh = view->handler_data;
    if (!vh->dragged_flag)
    {
      if (vh->resized_cell)
      {
        (*vh->on_resize)(view, vh->resized_cell->id, vh->resized_cell->view->frame.global.w);
      }
      else if (vh->dragged_cell)
      {
        (*vh->on_select)(view, vh->dragged_cell->id, ev);
      }
    }
    else
    {
      for (int i = 0; i < vh->cells->length; i++)
      {
        vh_lcell_t* cell = vh->cells->data[i];
        r2_t        f    = cell->view->frame.global;
        if (f.x < ev.x && f.x + f.w > ev.x && cell != vh->dragged_cell)
        {
          vh_lcell_ins(vh->cells, vh->dragged_ind, i);
          (*vh->on_insert)(view, vh->dragged_ind, i);
          break;
        }
      }
    }
    vh->dragged_cell = NULL;
    vh->resized_cell = NULL;
    vh->dragged_flag = 0;
    vh_lcell_arrange(vh->cells);
    vh_lhead_resize(view);
  }
}

void vh_lhead_resize(view_t* view)
{
  vh_lhead_t* vh = view->handler_data;

  vh_lcell_t* last   = vec_tail(vh->cells);
  r2_t        lframe = last->view->frame.local;
  r2_t        vframe = view->frame.local;
  vframe.w           = lframe.x + lframe.w;

  view_set_frame(view, vframe);
  view_layout(view);
}

// cell handling

void vh_lhead_add_cell(view_t* view, char* id, int size, view_t* cellview)
{
  vh_lhead_t* vh   = view->handler_data;
  vh_lcell_t* cell = vh_lcell_new(id, size, cellview, vh->cells->length); // REL 0

  // disable touch to enable mouse events
  cellview->needs_touch = 0;

  // add subview
  view_add_subview(view, cellview);

  // store cell
  VADD(vh->cells, cell);

  // arrange and resize
  vh_lcell_arrange(vh->cells);
  vh_lhead_resize(view);

  REL(cell);
}

view_t* vh_lhead_get_cell(view_t* view, char* id)
{
  vh_lhead_t* vh = view->handler_data;
  return vh_lcell_get(vh->cells, id);
}

#endif

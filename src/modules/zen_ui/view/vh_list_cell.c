#ifndef vh_list_cell_h
#define vh_list_cell_h

#include "view.c"

typedef struct _vh_lcell_t
{
  char*   id;
  int     size;
  view_t* view;
  int     index;
} vh_lcell_t;

vh_lcell_t* vh_lcell_new(char* id, int size, view_t* view, int index);
void        vh_lcell_arrange(vec_t* cells);
view_t*     vh_lcell_get(vec_t* cells, char* id);
void        vh_lcell_ins(vec_t* cells, int si, int ti);
void        vh_lcell_set_size(vec_t* cells, char* id, int size);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_cstring.c"
#include "zc_vector.c"

void vh_lcell_del(void* p)
{
  vh_lcell_t* cell = p;
  REL(cell->id);
}

void vh_lcell_desc(void* p, int level)
{
  printf("vh_lcell");
}

vh_lcell_t* vh_lcell_new(char* id, int size, view_t* view, int index)
{
  assert(view->handler == NULL && view->handler_data == NULL);

  vh_lcell_t* cell = CAL(sizeof(vh_lcell_t), vh_lcell_del, vh_lcell_desc);
  cell->id         = cstr_new_cstring(id); // REL 0
  cell->size       = size;
  cell->view       = view;
  cell->index      = index;

  return cell;
}

void vh_lcell_arrange(vec_t* cells)
{
  float pos = 0;
  for (int i = 0; i < cells->length; i++)
  {
    vh_lcell_t* cell = cells->data[i];
    cell->index      = i;
    r2_t f           = cell->view->frame.local;
    f.x              = pos;
    pos += f.w + 1;
    view_set_frame(cell->view, f);
  }
}

view_t* vh_lcell_get(vec_t* cells, char* id)
{
  for (int index = 0; index < cells->length; index++)
  {
    vh_lcell_t* cell = cells->data[index];
    if (strcmp(cell->id, id) == 0)
    {
      return cell->view;
    }
  }
  return NULL;
}

void vh_lcell_ins(vec_t* cells, int si, int ti)
{
  vh_lcell_t* cell = cells->data[si];
  RET(cell); // increase retain count for the swap
  VREM(cells, cell);
  vec_ins(cells, cell, ti);
  REL(cell);
  vh_lcell_arrange(cells);
}

void vh_lcell_set_size(vec_t* cells, char* id, int size)
{
  for (int index = 0; index < cells->length; index++)
  {
    vh_lcell_t* cell = cells->data[index];
    if (strcmp(cell->id, id) == 0)
    {
      r2_t f = cell->view->frame.local;
      f.w    = size;
      view_set_frame(cell->view, f);
      break;
    }
  }
  vh_lcell_arrange(cells);
}

#endif

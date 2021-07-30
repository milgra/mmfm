#ifndef column_h
#define column_h

typedef struct _col_t
{
  char* id;
  int   size;
  int   index;
} col_t;

col_t* col_new(char* id, int size, int index);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_cstring.c"
#include "zc_memory.c"

void col_del(void* p)
{
  col_t* col = p;
  REL(col->id);
}

void col_desc(void* p, int level)
{
  printf("col");
}

col_t* col_new(char* id, int size, int index)
{
  col_t* col = CAL(sizeof(col_t), col_del, col_desc);

  col->id    = cstr_new_cstring(id);
  col->size  = size;
  col->index = index;

  return col;
}

#endif

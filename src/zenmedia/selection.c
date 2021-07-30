#ifndef selection_h
#define selection_h

#include "zc_vector.c"

void selection_attach();
void selection_add(int i);
void selection_rem(int i);
void selection_rng(int i);
void selection_res();
int  selection_has(int i);
int  selection_cnt();
void selection_add_selection(vec_t* vec, vec_t* res);

#endif

#if __INCLUDE_LEVEL__ == 0

#include <stdio.h>

typedef struct _sel_rng_t
{
  int s;
  int e;
} sel_rng_t;

struct _selection_t
{
  sel_rng_t ranges[100];
  int       length;
} sel = {0};

void selection_add(int i)
{
  // check if song is in a range
  for (int index = 0; index < sel.length; index++)
  {
    sel_rng_t rng = sel.ranges[index];
    if (rng.s <= i && rng.e > i) return;
  }
  // store in a range
  sel.ranges[sel.length].s = i;
  sel.ranges[sel.length].e = i + 1;
  sel.length++;
}

void selection_rem(int i)
{
  // find songs range
  for (int index = 0; index < sel.length; index++)
  {
    sel_rng_t rng = sel.ranges[index];
    if (rng.s <= i && rng.e > i)
    {
      if (rng.s == i)
      {
        if (rng.e == i + 1)
        {
          // move indexes to invalid area
          sel.ranges[index].s = -1;
          sel.ranges[index].e = -1;
        }
        else
        {
          // exclude item
          sel.ranges[index].s += 1;
        }
      }
      else
      {
        // add a new range
        sel.ranges[index].e      = i;
        sel.ranges[sel.length].s = i + 1;
        sel.ranges[sel.length].e = rng.e;
        sel.length++;
      }
    }
  }
}

void selection_rng(int i)
{
  // extend last range
  if (i > sel.ranges[sel.length - 1].s)
    sel.ranges[sel.length - 1].e = i + 1;
  else
    sel.ranges[sel.length - 1].s = i;
}

void selection_res()
{
  sel.length = 0;
}

int selection_has(int i)
{
  // check if song is in a range
  for (int index = 0; index < sel.length; index++)
  {
    sel_rng_t rng = sel.ranges[index];
    if (rng.s <= i && rng.e > i) return 1;
  }
  return 0;
}

int selection_cnt()
{
  int cnt = 0;
  for (int index = 0; index < sel.length; index++)
  {
    sel_rng_t rng = sel.ranges[index];
    for (int r = rng.s; r < rng.e; r++)
    {
      cnt += rng.e - rng.s;
    }
  }
  return cnt;
}

void selection_add_selection(vec_t* vec, vec_t* res)
{
  for (int index = 0; index < sel.length; index++)
  {
    sel_rng_t rng = sel.ranges[index];
    for (int r = rng.s; r < rng.e; r++)
    {
      if (r >= 0 && r < vec->length)
      {
        VADD(res, vec->data[r]);
      }
    }
  }
}

#endif

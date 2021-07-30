#ifndef views_h
#define views_h

#include "zc_map.c"

typedef struct _views_t
{
  vec_t* list;
  map_t* names;
  int    arrange;
} views_t;

extern views_t views;

void views_init();
void views_destroy();
void views_describe();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "view.c"

views_t views = {0};

void views_init()
{
  views.list    = VNEW();
  views.names   = MNEW();
  views.arrange = 0;
}

void views_describe()
{
  int count = 0;
  for (int index = 0; index < views.list->length; index++)
  {
    view_t* view = views.list->data[index];
    if (mem_retaincount(view) > 2)
    {
      printf("view %s retc %zu\n", view->id, mem_retaincount(view));
      ++count;
    }
  }
  printf("total unreleased views : %i\n", count);
}

void views_destroy()
{
#ifdef DEBUG
  printf("***VIEW STATS***\n");
  printf("VIEW COUNT %i\n", views.list->length);
#endif
  // flatten view
  for (int index = 0; index < views.list->length; index++)
  {
    view_t* view = views.list->data[index];
    view_remove_from_parent(view);
    if (view->handler_data) REL(view->handler_data);
    if (view->tex_gen_data) REL(view->tex_gen_data);
    view->handler_data = NULL;
    view->tex_gen_data = NULL;
  }

  views_describe();

  // release list
  REL(views.list);
  REL(views.names);
  views.list  = NULL;
  views.names = NULL;
}

#endif

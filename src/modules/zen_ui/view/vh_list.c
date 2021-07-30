#ifndef vh_list_h
#define vh_list_h

#include "view.c"
#include "wm_event.c"
#include "zc_vector.c"

typedef struct _vh_list_inset_t
{
  float t;
  float b;
  float l;
  float r;
} vh_list_inset_t;

typedef struct _vh_list_t
{
  void* userdata;

  vec_t* items;

  int head_index; // index of top element
  int tail_index; // index of bottom element
  int top_index;  // index of upper visible element
  int bot_index;  // index of bottom visible element
  int item_count; // all elements in data source
  int full;       // list is full, no more elements needed

  char lock_scroll;

  float item_wth;     // width of all items
  float item_pos;     // horizontal position of all items
  float item_tgt_pos; // target item pos for animated horizontal scrol
  float head_pos;     // vertical position of head ( for refresh )

  // scrollers

  view_t* vscr;   // vertical scroller
  view_t* hscr;   // horizontal scroller
  view_t* header; // header view

  vh_list_inset_t inset; // scroll inset

  uint32_t vtimeout; // vertical scroller timeout
  uint32_t htimeout; // horizontal scroller timeout

  view_t* (*item_for_index)(int index, void* userdata, view_t* listview, int* item_count);
  void (*item_recycle)(view_t* item, void* userdata, view_t* listview);
} vh_list_t;

void    vh_list_add(view_t*         view,
                    vh_list_inset_t inset,
                    view_t* (*item_for_index)(int index, void* userdata, view_t* listview, int* item_count),
                    void (*item_recycle)(view_t* item, void* userdata, view_t* listview),
                    void* userdata);
void    vh_list_set_header(view_t* view, view_t* headerview);
vec_t*  vh_list_items(view_t* view);
vec_t*  vh_list_cache(view_t* view);
void    vh_list_fill(view_t* view);
void    vh_list_reset(view_t* view);
void    vh_list_refresh(view_t* view);
view_t* vh_list_item_for_index(view_t* view, int index);
void    vh_list_lock_scroll(view_t* view, char state);
void    vh_list_scroll_to_index(view_t* view, int index);
void    vh_list_scroll_to_x_poisiton(view_t* view, float position);
void    vh_list_set_item_width(view_t* view, float width);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "tg_css.c"
#include "tg_text.c"
#include "vh_anim.c"
#include "vh_sbar.c"
#include "zc_cstring.c"
#include "zc_string.c"
#include <float.h>
#include <limits.h>
#include <math.h>

#define PRELOAD_DISTANCE 100.0

void vh_list_update_scrollbars(view_t* view)
{
  vh_list_t* vh = view->handler_data;

  float sr; // size ratio
  float pr; // position ratio
  float s;  // size
  float p;  // position

  /* // vertical scroller */

  sr = 1.0;
  pr = 0.0;

  if (vh->bot_index - vh->top_index + 1 < vh->item_count)
  {
    sr = (float)(vh->bot_index - vh->top_index + 1) / (float)vh->item_count;
    pr = (float)(vh->top_index) / (float)(vh->item_count - (vh->bot_index - vh->top_index + 1));
  }

  vh_sbar_update(vh->vscr, pr, sr);

  // horizontal scroller

  sr = 1.0;
  pr = 0.0;

  if (vh->item_wth > view->frame.local.w)
  {
    sr = view->frame.local.w / vh->item_wth;
    pr = -vh->item_pos / (vh->item_wth - view->frame.global.w);
  }

  vh_sbar_update(vh->hscr, pr, sr);
}

void vh_list_move(view_t* view, float dy)
{
  vh_list_t* vh = view->handler_data;

  char inside  = 0;
  char outside = 0;

  for (int index = 0; index < vh->items->length; index++)
  {
    view_t* sview = vh->items->data[index];
    r2_t    frame = sview->frame.local;

    frame.x = vh->item_pos;
    frame.y += dy;

    view_set_frame(sview, frame);

    if (frame.y >= 0.0 && !inside)
    {
      inside        = 1;
      vh->top_index = vh->head_index + index;
    }

    if (frame.y >= view->frame.local.h && !outside)
    {
      outside       = 1;
      vh->bot_index = vh->head_index + index;
    }
  }

  if (vh->header)
  {
    r2_t frame = vh->header->frame.local;
    frame.x    = vh->item_pos;
    view_set_frame(vh->header, frame);
  }

  if (!outside) vh->bot_index = vh->tail_index;
  if (!inside) vh->top_index = vh->head_index;

  vh_list_update_scrollbars(view);
}

void vh_list_evt(view_t* view, ev_t ev)
{
  vh_list_t* vh = view->handler_data;
  if (ev.type == EV_TIME)
  {
    // fill up if needed
    while (vh->full == 0)
    {
      if (vh->items->length == 0)
      {
        view_t* item = (*vh->item_for_index)(vh->head_index, vh->userdata, view, &vh->item_count);

        if (item)
        {
          uint32_t index = vec_index_of_data(vh->items, item);
          if (index == UINT32_MAX)
          {
            VADD(vh->items, item);
            view_insert_subview(view, item, 0);
          }
          else
            printf("VH LIST EMPTY : ITEM ALREADY IN ITEMS %s %i\n", item->id, vh->head_index);

          view_set_frame(item, (r2_t){0, vh->head_pos, item->frame.local.w, item->frame.local.h});

          vh->item_wth = item->frame.global.w; // store maximum width
        }
        else
          vh->full = 1;
      }
      else
      {
        view_t* head = vec_head(vh->items);
        view_t* tail = vec_tail(vh->items);

        // add items if needed

        if (head->frame.local.y > 0.0 - PRELOAD_DISTANCE)
        {
          view_t* item = (*vh->item_for_index)(vh->head_index - 1, vh->userdata, view, &vh->item_count);

          if (item)
          {
            uint32_t index = vec_index_of_data(vh->items, item);

            if (index == UINT32_MAX)
            {
              vec_ins(vh->items, item, 0);
              view_insert_subview(view, item, 0);
            }
            else
              printf("VH LIST TOP : ITEM ALREADY IN ITEMS %s index %i\n", item->id, vh->head_index - 1);

            vh->full     = 0;                    // there is probably more to come
            vh->item_wth = item->frame.global.w; // store maximum width
            vh->head_index -= 1;                 // decrease head index

            view_set_frame(item, (r2_t){0, head->frame.local.y - item->frame.local.h, item->frame.local.w, item->frame.local.h});
          }
          else
          {
            vh->full = 1;
          }
        }
        else
        {
          vh->full = 1;
        }

        if (tail->frame.local.y + tail->frame.local.h < view->frame.local.h + PRELOAD_DISTANCE)
        {
          view_t* item = (*vh->item_for_index)(vh->tail_index + 1, vh->userdata, view, &vh->item_count);

          if (item)
          {
            uint32_t index = vec_index_of_data(vh->items, item);

            if (index == UINT32_MAX)
            {
              VADD(vh->items, item);
              view_insert_subview(view, item, 0);
            }
            else
              printf("VH LIST BOTTOM : ITEM ALREADY IN ITEMS %s index %i\n", item->id, vh->tail_index + 1);

            vh->full     = 0;                    // there is probably more to come
            vh->item_wth = item->frame.global.w; // store maximum width
            vh->tail_index += 1;                 // increase tail index

            view_set_frame(item, (r2_t){0, tail->frame.local.y + tail->frame.local.h, item->frame.local.w, item->frame.local.h});
          }
          else
          {
            vh->full &= 1; // don't set to full if previously item is added
          }
        }
        else
        {
          vh->full &= 1; // don't set to full if previously item is added
        }

        // remove items if needed

        if (tail->frame.local.y - (head->frame.local.y + head->frame.local.h) > view->frame.local.h)
        {
          if (head->frame.local.y + head->frame.local.h < 0.0 - PRELOAD_DISTANCE && vh->items->length > 1)
          {
            VREM(vh->items, head);
            vh->head_index += 1;
            view_remove_from_parent(head);
            if (vh->item_recycle) (*vh->item_recycle)(head, vh->userdata, view);
          }
          if (tail->frame.local.y > view->frame.local.h + PRELOAD_DISTANCE && vh->items->length > 1)
          {
            VREM(vh->items, tail);
            vh->tail_index -= 1;
            view_remove_from_parent(tail);
            if (vh->item_recycle) (*vh->item_recycle)(tail, vh->userdata, view);
          }
        }
      }
    }
    // scroll bounce if needed
    if (vh->items->length > 0 && !vh->lock_scroll)
    {
      view_t* head = vec_head(vh->items);
      view_t* tail = vec_tail(vh->items);

      char move = 0;
      // horizontal bounce

      if (vh->item_pos > 0.0001) // left overflow
      {
        vh->item_pos += -vh->item_pos / 5.0;
        move = 1;
      }
      else if (vh->item_pos + vh->item_wth < view->frame.local.w - vh->inset.r) // right overflow
      {
        if (vh->item_wth > view->frame.local.w - vh->inset.r)
        {
          vh->item_pos += (view->frame.local.w - vh->inset.r - vh->item_wth - vh->item_pos) / 5.0;
          move = 1;
        }
        else if (vh->item_pos < -0.0001)
        {
          vh->item_pos += -vh->item_pos / 5.0;
          move = 1;
        }
      }
      else if (vh->item_tgt_pos != FLT_MAX)
      {
        if (vh->item_tgt_pos < vh->item_pos - 0.0001 || vh->item_tgt_pos > vh->item_pos + 0.0001)
        {
          vh->item_pos += (vh->item_tgt_pos - vh->item_pos) / 4;
          move = 1;
        }
        else
          vh->item_tgt_pos = FLT_MAX;
      }

      if (move) vh_list_move(view, 0);

      // vertical bounce

      if (head->frame.local.y > vh->inset.t + 0.001)
      {
        vh_list_move(view, (vh->inset.t - head->frame.local.y) / 5.0);
      }
      else if (tail->frame.local.y + tail->frame.local.h < view->frame.local.h - 0.001 - vh->inset.b)
      {
        if (tail->frame.local.y + tail->frame.local.h - head->frame.local.y > view->frame.local.h - vh->inset.b)
        {
          vh_list_move(view, (view->frame.local.h - vh->inset.b - (tail->frame.local.y + tail->frame.local.h)) / 5.0);
        }
        else if (head->frame.local.y < -0.001)
        {
          vh_list_move(view, -head->frame.local.y / 5.0);
        }
      }
    }
    // close scrollbars
    if (vh->vtimeout > 0 && vh->vtimeout < ev.time)
    {
      vh->vtimeout = 0;
      vh_sbar_close(vh->vscr);
    }
    if (vh->htimeout > 0 && vh->htimeout < ev.time)
    {
      vh->htimeout = 0;
      vh_sbar_close(vh->hscr);
    }

    vh->bot_index = vh->tail_index;
    vh->top_index = vh->head_index;
    vh_list_update_scrollbars(view);
  }
  else if (ev.type == EV_SCROLL)
  {
    if (vh->items->length > 0 && !vh->lock_scroll)
    {
      if (ev.dx != 0.0)
      {
        vh->item_pos += ev.dx;

        if (vh->htimeout == 0)
        {
          vh->htimeout = ev.time + 1000;
          vh_sbar_open(vh->hscr);
        }
        else
          vh->htimeout = ev.time + 1000;
      }

      if (ev.dy != 0.0)
      {
        if (ev.dy > 100.0) ev.dy = 100.0;
        if (ev.dy < -100.0) ev.dy = -100.0;

        vh_list_move(view, ev.dy);
        vh->full = 0;

        if (vh->vtimeout == 0)
        {
          vh->vtimeout = ev.time + 1000;
          vh_sbar_open(vh->vscr);
        }
        else
          vh->vtimeout = ev.time + 1000;
      }
      else if (ev.dx != 0)
        vh_list_move(view, 0);
    }
  }
  else if (ev.type == EV_RESIZE)
  {
    vh->full = 0;
  }
}

vec_t* vh_list_items(view_t* view)
{
  vh_list_t* vh = view->handler_data;
  return vh->items;
}

void vh_list_fill(view_t* view)
{
  vh_list_t* vh = view->handler_data;
  vh->full      = 0;
}

void vh_list_lock_scroll(view_t* view, char state)
{
  vh_list_t* vh   = view->handler_data;
  vh->lock_scroll = state;
}

void vh_list_scroll_v(view_t* view, void* userdata, float ratio)
{
  view_t*    listview  = userdata;
  vh_list_t* vh        = listview->handler_data;
  int        new_index = (int)((float)vh->item_count * ratio);

  if (new_index != vh->head_index && new_index > 0 && new_index < vh->item_count)
  {
    vh->head_index = new_index;
    vh_list_refresh(listview);
  }
}

void vh_list_scroll_h(view_t* view, void* userdata, float ratio)
{
  view_t*    listview = userdata;
  vh_list_t* vh       = listview->handler_data;
  vh->item_pos        = -vh->item_wth * ratio;

  vh_list_move(listview, 0);
}

void vh_list_scroll_to_index(view_t* view, int index)
{
  vh_list_t* vh = view->handler_data;

  if (index < vh->item_count)
  {
    vh->head_index = index;
    vh_list_refresh(view);
    vh->head_pos = vh->header ? vh->header->frame.local.h : 0;
    vh_list_update_scrollbars(view);
  }
}

void vh_list_scroll_to_x_poisiton(view_t* view, float position)
{
  vh_list_t* vh = view->handler_data;

  if (position > 0) position = 0;
  vh->item_tgt_pos = position;
  vh_list_move(view, 0);
}

void vh_list_set_item_width(view_t* view, float width)
{
  vh_list_t* vh = view->handler_data;
  vh->item_wth  = width;
}

void vh_list_reset(view_t* view)
{
  vh_list_t* vh = view->handler_data;

  for (int index = 0; index < vh->items->length; index++)
  {
    view_t* item = vh->items->data[index];
    view_remove_from_parent(item);
    if (vh->item_recycle) (*vh->item_recycle)(item, vh->userdata, view);
  }

  vec_reset(vh->items);

  vh->head_index = 0;
  vh->tail_index = 0;
  vh->item_count = 0;
  vh->full       = 0;
}

void vh_list_refresh(view_t* view)
{
  vh_list_t* vh = view->handler_data;

  for (int index = 0; index < vh->items->length; index++)
  {
    view_t* item = vh->items->data[index];
    if (index == 0) vh->head_pos = item->frame.local.y;
    view_remove_from_parent(item);
    if (vh->item_recycle) (*vh->item_recycle)(item, vh->userdata, view);
  }

  vec_reset(vh->items);

  vh->full       = 0;
  vh->tail_index = vh->head_index;
}

void vh_list_del(void* p)
{
  vh_list_t* vh = p;
  REL(vh->items);
}

void vh_list_desc(void* p, int level)
{
  printf("vh_list");
}

void vh_list_add(view_t*         view,
                 vh_list_inset_t inset,
                 view_t* (*item_for_index)(int index, void* userdata, view_t* listview, int* item_count),
                 void (*item_recycle)(view_t* item, void* userdata, view_t* listview),
                 void* userdata)
{
  assert(view->handler == NULL && view->handler_data == NULL);

  vh_list_t* vh      = CAL(sizeof(vh_list_t), vh_list_del, vh_list_desc);
  vh->userdata       = userdata;
  vh->items          = VNEW(); // REL 0
  vh->item_for_index = item_for_index;
  vh->item_recycle   = item_recycle;
  vh->inset          = inset;
  vh->item_tgt_pos   = FLT_MAX;

  char* vid = cstr_new_format(100, "%s%s", view->id, "vscr");
  char* hid = cstr_new_format(100, "%s%s", view->id, "hscr");

  view_t* vscr = view_new(vid, (r2_t){view->frame.local.w - 21, 0, 21, view->frame.local.h}); // REL 1
  view_t* hscr = view_new(hid, (r2_t){0, view->frame.local.h - 21, view->frame.local.w, 21}); // REL 2

  REL(vid);
  REL(hid);

  vscr->layout.h_per  = 1.0;
  vscr->layout.right  = 0;
  hscr->layout.w_per  = 1.0;
  hscr->layout.bottom = 0;

  vscr->layout.background_color = 0x00000001;
  hscr->layout.background_color = 0x00000001;

  tg_css_add(vscr);
  tg_css_add(hscr);

  vh_sbar_add(vscr, SBAR_V, 30, 10, vh_list_scroll_v, view);
  vh_sbar_add(hscr, SBAR_H, 30, 10, vh_list_scroll_h, view);

  view_add_subview(view, vscr);
  view_add_subview(view, hscr);

  vh->vscr = vscr;
  vh->hscr = hscr;

  view->handler_data = vh;
  view->handler      = vh_list_evt;

  view->blocks_scroll = 1;

  REL(vh->vscr);
  REL(vh->hscr);
}

view_t* vh_list_item_for_index(view_t* view, int index)
{
  vh_list_t* vh = view->handler_data;

  if (index > vh->head_index && index < vh->tail_index)
  {
    int diff = index - vh->head_index;
    return vh->items->data[diff];
  }

  return NULL;
}

void vh_list_set_header(view_t* view, view_t* headerview)
{
  vh_list_t* vh = view->handler_data;

  if (vh->header != NULL) view_remove_from_parent(vh->header);
  vh->header = headerview;

  // add as subview before scrollers
  view_insert_subview(view, headerview, 0);

  vh->head_pos = headerview->frame.local.h;
}

#endif

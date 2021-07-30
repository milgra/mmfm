// wraps a textinput into a list so editing will be scrollable

#ifndef vh_textinput_scroller_h
#define vh_textinput_scroller_h

#include "ui_manager.c"
#include "vh_button.c"
#include "vh_list.c"
#include "vh_textinput.c"
#include "view.c"

typedef struct _vh_textinput_scroller_t
{
  view_t* list_v;
  view_t* input_txt_v;
  view_t* clear_btn_v;
} vh_textinput_scroller_t;

void vh_textinput_scroller_add(view_t*     view,
                               char*       text,
                               char*       phtext,
                               textstyle_t textstyle,
                               void*       userdata);

view_t* vh_textinput_scroller_get_input_view(view_t* view);

void   vh_textinput_scroller_activate(view_t* view, int state);
str_t* vh_textinput_scroller_get_text(view_t* view);

#endif

#if __INCLUDE_LEVEL__ == 0

view_t* vh_textinput_scroller_item_for_index(int index, void* userdata, view_t* listview, int* item_count);
void    vh_textinput_scroller_on_text(view_t* textview, void* userdata);
void    vh_textinput_scroller_on_button_down(void* userdata, void* data);

void vh_textinput_scroller_del(void* p)
{
  vh_textinput_scroller_t* vh = p;
}

void vh_textinput_scroller_desc(void* p, int level)
{
  printf("vh_textinput_scroller");
}

void vh_textinput_scroller_add(view_t*     view,
                               char*       text,
                               char*       phtext,
                               textstyle_t textstyle,
                               void*       userdata)
{
  assert(view->handler == NULL && view->handler_data == NULL);

  vh_textinput_scroller_t* data = CAL(sizeof(vh_textinput_scroller_t), vh_textinput_scroller_del, vh_textinput_scroller_desc);

  assert(view->views->length == 3);

  view_t* list_v          = view->views->data[0];
  view_t* input_txt_v     = view->views->data[1];
  view_t* clear_btn_bck_v = view->views->data[2]; // TODO do we need btn bck?
  view_t* clear_btn_v     = clear_btn_bck_v->views->data[0];
  cb_t*   cb_btn_press    = cb_new(vh_textinput_scroller_on_button_down, view); // REL 0

  vh_button_add(clear_btn_v, VH_BUTTON_NORMAL, cb_btn_press);
  vh_textinput_add(input_txt_v, text, phtext, textstyle, view);
  vh_textinput_set_on_text(input_txt_v, vh_textinput_scroller_on_text);
  vh_list_add(list_v, ((vh_list_inset_t){0}), vh_textinput_scroller_item_for_index, NULL, view);

  data->list_v      = list_v;
  data->input_txt_v = input_txt_v;
  data->clear_btn_v = clear_btn_v;

  view_remove_from_parent(input_txt_v); // remove input txt view, list view will add it as row

  view->handler_data = data;

  REL(cb_btn_press); // REL 0
}

view_t* vh_textinput_scroller_item_for_index(int index, void* userdata, view_t* listview, int* item_count)
{
  view_t*                  view = userdata;
  vh_textinput_scroller_t* data = view->handler_data;

  if (index == 0)
    return data->input_txt_v;
  else
    return NULL;
}

void vh_textinput_scroller_on_text(view_t* textview, void* userdata)
{
  view_t*                  view = userdata;
  vh_textinput_scroller_t* data = view->handler_data;

  if (data->input_txt_v->frame.local.w > data->list_v->frame.local.w)
  {
    vh_list_set_item_width(data->list_v, data->input_txt_v->frame.local.w);
    vh_list_scroll_to_x_poisiton(data->list_v, data->list_v->frame.local.w - data->input_txt_v->frame.local.w);
  }
}

void vh_textinput_scroller_on_button_down(void* userdata, void* btndata)
{
  view_t*                  view = userdata;
  vh_textinput_scroller_t* data = view->handler_data;

  vh_list_set_item_width(data->list_v, data->input_txt_v->frame.local.w);
  vh_list_scroll_to_x_poisiton(data->list_v, 0);
  vh_textinput_set_text(data->input_txt_v, "");
  vh_textinput_activate(data->input_txt_v, 1);
  ui_manager_activate(data->input_txt_v);
}

view_t* vh_textinput_scroller_get_input_view(view_t* view)
{
  vh_textinput_scroller_t* data = view->handler_data;
  return data->input_txt_v;
}

void vh_textinput_scroller_activate(view_t* view, int state)
{
  vh_textinput_scroller_t* data = view->handler_data;
  vh_textinput_activate(data->input_txt_v, 1);
  ui_manager_activate(data->input_txt_v);
}

str_t* vh_textinput_scroller_get_text(view_t* view)
{
  vh_textinput_scroller_t* data = view->handler_data;
  return vh_textinput_get_text(data->input_txt_v);
}

#endif

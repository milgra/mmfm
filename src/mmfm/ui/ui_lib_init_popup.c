#ifndef ui_lib_init_popup_h
#define ui_lib_init_popup_h

#include "view.c"

void ui_lib_init_popup_attach(view_t* baseview);
void ui_lib_init_popup_detach();
void ui_lib_init_popup_show(char* text);
void ui_lib_init_popup_hide();
void ui_lib_init_popup_set_library();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "callbacks.c"
#include "config.c"
#include "tg_text.c"
#include "ui_manager.c"
#include "vh_button.c"
#include "vh_list.c"
#include "vh_textinput.c"
#include "vh_textinput_scroller.c"
#include "wm_connector.c"
#include "zc_callback.c"
#include "zc_text.c"

struct _ui_lib_init_popup_t
{
  char*   fontpath;
  view_t* baseview;
  view_t* textfield;
  view_t* textinput;
  view_t* view;
} ulip = {0};

void    ui_lib_init_on_button_down(void* userdata, void* data);
void    ui_lib_init_popup_set_library(view_t* view);
void    ui_lib_init_popup_on_text(view_t* view);
view_t* ui_lib_init_popup_item_for_index(int index, void* userdata, view_t* listview, int* item_count);

void ui_lib_init_popup_attach(view_t* baseview)
{
  ulip.fontpath  = config_get("font_path");
  ulip.baseview  = baseview;
  ulip.view      = view_get_subview(baseview, "lib_init_page");
  ulip.textfield = view_get_subview(baseview, "lib_init_textfield");

  view_t* textinput_scroller = view_get_subview(baseview, "lib_init_textinput_scroller");

  ulip.textinput = textinput_scroller;

  cb_t* cb_btn_press = cb_new(ui_lib_init_on_button_down, NULL); // REL 0

  vh_button_add(view_get_subview(baseview, "lib_init_accept_btn"), VH_BUTTON_NORMAL, cb_btn_press);
  vh_button_add(view_get_subview(baseview, "lib_init_reject_btn"), VH_BUTTON_NORMAL, cb_btn_press);

  textstyle_t ts  = {0};
  ts.font         = ulip.fontpath;
  ts.align        = TA_LEFT;
  ts.margin_left  = 10;
  ts.margin_right = 10;
  ts.size         = 30.0;
  ts.textcolor    = 0x000000FF;
  ts.backcolor    = 0;
  ts.autosize     = AS_AUTO;

  tg_text_add(ulip.textfield);

  ts.backcolor = 0xFFFFFFFF;

  vh_textinput_scroller_add(textinput_scroller, "/home/youruser/Music", "", ts, NULL);

  vh_textinput_set_on_return(vh_textinput_scroller_get_input_view(textinput_scroller), ui_lib_init_popup_set_library);

  view_remove_from_parent(ulip.view);

  REL(cb_btn_press); // REL 0
}

void ui_lib_init_popup_detach()
{
  if (ulip.view->parent)
  {
    ui_manager_remove(ulip.view);
    ui_manager_add(ulip.baseview);
  }
  view_add_subview(ulip.baseview, ulip.view);
}

void ui_lib_init_popup_set_library(view_t* view)
{
  // get path string
  str_t* path    = vh_textinput_scroller_get_text(ulip.textinput);
  char*  path_ch = str_new_cstring(path); // REL 0

  callbacks_call("on_change_library", path_ch);
  REL(path_ch); // REL 0
}

void ui_lib_init_on_button_down(void* userdata, void* data)
{
  char* id = ((view_t*)data)->id;

  if (strcmp(id, "lib_init_accept_btn") == 0) ui_lib_init_popup_set_library(NULL);
  if (strcmp(id, "lib_init_reject_btn") == 0) wm_close();
}

void ui_lib_init_popup_show(char* text)
{
  textstyle_t ts = {0};
  ts.font        = ulip.fontpath;
  ts.align       = TA_CENTER;
  ts.size        = 30.0;
  ts.textcolor   = 0x000000FF;
  ts.backcolor   = 0;

  tg_text_set(ulip.textfield, text, ts);

  if (!ulip.view->parent)
  {
    ui_manager_remove(ulip.baseview);
    ui_manager_add(ulip.view);
  }

  vh_textinput_scroller_activate(ulip.textinput, 1); // activate text input
}

void ui_lib_init_popup_hide()
{
  if (ulip.view->parent)
  {
    ui_manager_remove(ulip.view);
    ui_manager_add(ulip.baseview);
  }
}

#endif

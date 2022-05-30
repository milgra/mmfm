#ifndef ui_inputfield_popup_h
#define ui_inputfield_popup_h

#include "view.c"
#include "zc_callback.c"

void ui_inputfield_popup_attach(view_t* baseview);
void ui_inputfield_popup_detach();

void ui_inputfield_popup_show(char* text, cb_t* acc_cb, cb_t* rej_cb);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "tg_text.c"
#include "ui_manager.c"
#include "ui_popup_switcher.c"
#include "vh_button.c"
#include "vh_list.c"
#include "vh_textinput.c"
#include "vh_textinput_scroller.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_vector.c"

void    ui_inputfield_popup_accept(void* userdata, void* data);
void    ui_inputfield_popup_reject(void* userdata, void* data);
void    ui_inputfield_popup_enter(view_t* view);
void    ui_inputfield_on_button_down(void* userdata, void* data);
view_t* ui_inputfield_item_for_index(int index, void* userdata, view_t* listview, int* item_count);
void    ui_inputfield_on_text(view_t* view, void* userdata);

struct _ui_inputfield_popup_t
{
  char        attached;
  vec_t*      requests;
  view_t*     textfield;
  view_t*     textinput;
  textstyle_t textstyle;
} uip = {0};

void ui_inputfield_popup_attach(view_t* baseview)
{
  if (uip.attached) return;

  view_t* textfield = view_get_subview(baseview, "inp_popup_textfield");

  if (textfield)
  {

    textstyle_t ts = {0};
    ts.font        = config_get("font_path");
    ts.size        = 30.0;
    ts.margin      = 5;
    ts.align       = TA_LEFT;
    ts.textcolor   = 0x000000FF;
    ts.backcolor   = 0;
    ts.multiline   = 0;
    ts.autosize    = AS_AUTO;

    view_t* acc_btn   = view_get_subview(baseview, "inp_popup_accept_btn");
    view_t* rej_btn   = view_get_subview(baseview, "inp_popup_reject_btn");
    view_t* textinput = view_get_subview(baseview, "inp_popup_textinput_scroller");

    vh_textinput_scroller_add(textinput, "/home/youruser/Music", "", ts, NULL);
    vh_textinput_set_on_return(vh_textinput_scroller_get_input_view(textinput), ui_inputfield_popup_enter);

    tg_text_add(textfield);

    ts.autosize  = AS_FIX;
    ts.multiline = 1;
    ts.align     = TA_CENTER;

    cb_t* cb_btn_press = cb_new(ui_inputfield_on_button_down, NULL); // REL 0

    vh_button_add(acc_btn, VH_BUTTON_NORMAL, cb_btn_press);
    vh_button_add(rej_btn, VH_BUTTON_NORMAL, cb_btn_press);

    uip.textstyle = ts;
    uip.textfield = textfield;
    uip.textinput = textinput;
    uip.attached  = 1;
    uip.requests  = VNEW(); // REL 1

    REL(cb_btn_press); // REL 0
  }
  else
    zc_log_debug("inp_popup_textfield not found");
}

void ui_inputfield_popup_detach()
{
  REL(uip.requests);
}

void ui_inputfield_on_button_down(void* userdata, void* data)
{
  char* id = ((view_t*)data)->id;

  if (strcmp(id, "inp_popup_accept_btn") == 0) ui_inputfield_popup_accept(NULL, NULL);
  if (strcmp(id, "inp_popup_reject_btn") == 0) ui_inputfield_popup_reject(NULL, NULL);
}

void ui_inputfield_popup_shownext()
{
  if (uip.requests->length > 0)
  {
    map_t* request = vec_tail(uip.requests);
    char*  text    = MGET(request, "text");
    tg_text_set(uip.textfield, text, uip.textstyle);
  }
  else
    ui_popup_switcher_toggle("inp_popup_page");
}

void ui_inputfield_popup_show(char* text, cb_t* acc_cb, cb_t* rej_cb)
{
  map_t* request = MNEW(); // REL 0

  MPUTR(request, "text", cstr_new_cstring(text));

  if (acc_cb) MPUT(request, "acc_cb", acc_cb);
  if (rej_cb) MPUT(request, "rej_cb", rej_cb);

  VADD(uip.requests, request);

  tg_text_set(uip.textfield, text, uip.textstyle);
  ui_popup_switcher_toggle("inp_popup_page");

  vh_textinput_scroller_activate(uip.textinput, 1); // activate text input

  REL(request);
}

void ui_inputfield_popup_enter(view_t* view)
{
  ui_inputfield_popup_accept(NULL, NULL);
}

void ui_inputfield_popup_accept(void* userdata, void* data)
{
  map_t* request = vec_tail(uip.requests);
  if (request)
  {
    // char* text   = MGET(request, "text");
    cb_t* acc_cb = MGET(request, "acc_cb");

    str_t* inputstr = vh_textinput_scroller_get_text(uip.textinput);

    if (acc_cb) (*acc_cb->fp)(acc_cb->userdata, inputstr);
    vec_rem(uip.requests, request);
  }

  ui_inputfield_popup_shownext();
}

void ui_inputfield_popup_reject(void* userdata, void* data)
{
  map_t* request = vec_tail(uip.requests);
  if (request)
  {
    // char* text   = MGET(request, "text");
    cb_t* rej_cb = MGET(request, "rej_cb");

    if (rej_cb) (*rej_cb->fp)(rej_cb->userdata, NULL);
    vec_rem(uip.requests, request);
  }

  ui_inputfield_popup_shownext();
}

#endif

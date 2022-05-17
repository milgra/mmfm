#ifndef ui_alert_popup_h
#define ui_alert_popup_h

#include "view.c"
#include "zc_callback.c"

void ui_alert_popup_attach(view_t* baseview);
void ui_alert_popup_detach();
void ui_alert_popup_show(char* text);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "tg_text.c"
#include "ui_popup_switcher.c"
#include "vh_button.c"
#include "zc_text.c"
#include "zc_vector.c"

void ui_alert_popup_accept(void* userdata, void* data);

struct _ui_alert_popup_t
{
  view_t* sim_pop_txt;
  char*   fontpath;
  vec_t*  textqueue;
} uap = {0};

void ui_alert_popup_attach(view_t* baseview)
{
  uap.sim_pop_txt = view_get_subview(baseview, "sim_pop_txt");
  tg_text_add(uap.sim_pop_txt);
  uap.fontpath  = config_get("font_path");
  uap.textqueue = VNEW(); // GREL 0

  view_t* acc_btn = view_get_subview(baseview, "simple_pop_acc_btn");
  cb_t*   acc_cb  = cb_new(ui_alert_popup_accept, NULL); // REL 1
  vh_button_add(acc_btn, VH_BUTTON_NORMAL, acc_cb);

  REL(acc_cb); // REL 1
}

void ui_alert_popup_detach()
{
  REL(uap.textqueue); // GREL 0
}

void ui_alert_popup_show(char* text)
{
  textstyle_t ts = {0};
  ts.font        = uap.fontpath;
  ts.align       = TA_CENTER;
  ts.size        = 30.0;
  ts.textcolor   = 0x000000FF;
  ts.backcolor   = 0;

  tg_text_set(uap.sim_pop_txt, text, ts);

  ui_popup_switcher_toggle("simple_popup_page");
}

void ui_alert_popup_accept(void* userdata, void* data)
{
  ui_popup_switcher_toggle("simple_popup_page");
}

#endif

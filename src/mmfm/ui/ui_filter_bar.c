#ifndef ui_filter_bar_h
#define ui_filter_bar_h

#include "view.c"

void ui_filter_bar_attach(view_t* baseview);
void ui_filter_bar_detach();
void ui_filter_bar_clear_search();
void ui_filter_bar_show_query(char* text);

#endif

#if __INCLUDE_LEVEL__ == 0

void ui_filter_bar_show_filters(void* userdata, void* data);
void ui_filter_bar_clear_search(void* userdata, void* data);

#include "config.c"
#include "ui_filelist.c"
#include "ui_filter_popup.c"
#include "ui_manager.c"
#include "vh_button.c"
#include "vh_textinput.c"
#include "visible.c"
#include "zc_log.c"

void ui_filter_bar_filter(view_t* view, void* userdata);

struct _ui_filter_bar_t
{
  view_t* filelist_filter_bar;

} ufb = {0};

void ui_filter_bar_attach(view_t* baseview)
{
  ufb.filelist_filter_bar = view_get_subview(baseview, "filterfield");

  if (ufb.filelist_filter_bar)
  {
    textstyle_t ts  = {0};
    ts.font         = config_get("font_path");
    ts.size         = 30.0;
    ts.margin_right = 20;
    ts.align        = TA_LEFT;
    ts.textcolor    = 0x000000FF;
    ts.backcolor    = 0xFFFF00FF;

    view_t* clearbtn  = view_get_subview(baseview, "clearbtn");
    view_t* filterbtn = view_get_subview(baseview, "filterbtn");

    cb_t* filter_cb = cb_new(ui_filter_bar_show_filters, NULL); // REL 0
    cb_t* clear_cb  = cb_new(ui_filter_bar_clear_search, NULL); // REL 1

    vh_button_add(filterbtn, VH_BUTTON_NORMAL, filter_cb);
    vh_button_add(clearbtn, VH_BUTTON_NORMAL, clear_cb);

    REL(filter_cb); // REL 0
    REL(clear_cb);  // REL 1

    vh_textinput_add(ufb.filelist_filter_bar, "", "Search/Filter", ts, NULL);
    vh_textinput_set_on_text(ufb.filelist_filter_bar, ui_filter_bar_filter);
    // vh_textinput_set_on_activate(ufb.filelist_filter_bar, ui_on_filter_activate);
  }
  else
    zc_log_debug("filterfield not found");
}

void ui_filter_bar_detach()
{
}

void ui_filter_bar_filter(view_t* view, void* userdata)
{
  str_t* text = vh_textinput_get_text(view);

  char* ctext = str_new_cstring(text); // REL 0

  visible_set_filter(ctext);
  ui_filelist_update();

  REL(ctext); // REL 0
}

void ui_filter_bar_show_filters(void* userdata, void* data)
{
  ui_filter_popup_show();
}
void ui_filter_bar_clear_search(void* userdata, void* data)
{
  vh_textinput_set_text(ufb.filelist_filter_bar, "");
  ui_manager_activate(ufb.filelist_filter_bar);
  vh_textinput_activate(ufb.filelist_filter_bar, 1);
}

void ui_filter_bar_show_query(char* text)
{
  vh_textinput_set_text(ufb.filelist_filter_bar, text);
}

#endif

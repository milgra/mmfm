#ifndef ui_h
#define ui_h

#include "view.c"

void ui_init(float width, float height);
void ui_destroy();
void ui_add_cursor();
void ui_update_cursor(r2_t frame);
void ui_render_without_cursor(uint32_t time);
void ui_save_screenshot(uint32_t time, char hide_cursor);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "bm_rgba_util.c"
#include "callbacks.c"
#include "coder.c"
#include "config.c"
#include "player.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_about_popup.c"
#include "ui_activity_popup.c"
#include "ui_alert_popup.c"
#include "ui_cliplist.c"
#include "ui_compositor.c"
#include "ui_decision_popup.c"
#include "ui_filelist.c"
#include "ui_filter_bar.c"
#include "ui_filter_popup.c"
#include "ui_inputfield_popup.c"
#include "ui_lib_init_popup.c"
#include "ui_manager.c"
#include "ui_meta_view.c"
#include "ui_play_controls.c"
#include "ui_popup_switcher.c"
#include "ui_settings_popup.c"
#include "ui_song_infos.c"
#include "ui_song_menu_popup.c"
#include "ui_visualizer.c"
#include "vh_button.c"
#include "vh_key.c"
#include "view_generator.c"
#include "view_layout.c"
#include "wm_connector.c"
#include "zc_log.c"
#include "zc_number.c"
#include "zc_path.c"

view_t* view_base;
vec_t*  view_list;
view_t* rep_cur; // replay cursor

void ui_on_button_down(void* userdata, void* data);
void ui_on_key_down(void* userdata, void* data);

void ui_init(float width, float height)
{
  text_init();                    // DESTROY 0
  ui_manager_init(width, height); // DESTROY 1

  // callbacks setup for simple buttons

  cb_t* button_down = cb_new(ui_on_button_down, NULL); // REL 0
  callbacks_set("on_button_press", button_down);
  REL(button_down); // REL 0

  // view setup with existing callbacks

  view_list = view_gen_load(config_get("html_path"), config_get("css_path"), config_get("res_path"), callbacks_get_data()); // REL 0
  view_base = vec_head(view_list);

  zc_log_debug("%i views generated", view_list->length);

  // initial layout of views

  view_set_frame(view_base, (r2_t){0.0, 0.0, (float)width, (float)height});
  view_layout(view_base);

  ui_manager_add(view_base);

  // attach ui components

  ui_filelist_attach(view_base); // DETACH 0
  /* ui_cliplist_attach(view_base); // DETACH 0 */
  ui_meta_view_attach(view_base);
  /* ui_song_infos_attach(view_base);       // DETACH 1 */
  ui_visualizer_attach(view_base); // DETACH 2
  ui_filter_bar_attach(view_base); // DETACH 3
  /* ui_about_popup_attach(view_base);      // DETACH 4 */
  /* ui_alert_popup_attach(view_base);      // DETACH 5 */
  /* ui_filter_popup_attach(view_base);     // DETACH 6 */
  ui_play_controls_attach(view_base); // DETACH 8
  /* ui_decision_popup_attach(view_base);   // DETACH 9 */
  /* ui_lib_init_popup_attach(view_base);   // DETACH 10 */
  /* ui_activity_popup_attach(view_base);   // DETACH 11 */
  /* ui_settings_popup_attach(view_base);   // DETACH 12 */
  ui_song_menu_popup_attach(view_base); // DETACH 13
  /* ui_inputfield_popup_attach(view_base); // DETACH 14 */

  // setup views

  view_t* main_view = view_get_subview(view_base, "main");
  /* view_t* song_info = view_get_subview(main_view, "song_info"); */

  /* cb_t* key_cb = cb_new(ui_on_key_down, view_base); // REL 1 */
  /* cb_t* but_cb = cb_new(ui_on_button_down, NULL);   // REL 2 */

  main_view->needs_touch = 0; // don't cover events from filelist
  /* vh_key_add(view_base, key_cb);                      // listen on view_base for shortcuts */
  /* vh_button_add(song_info, VH_BUTTON_NORMAL, but_cb); // show messages on song info click */

  // finally attach and remove popups, it removes views so it has to be the last command

  ui_popup_switcher_attach(view_base); // DETACH 15

  textstyle_t ts  = {0};
  ts.font         = config_get("font_path");
  ts.align        = TA_CENTER;
  ts.margin_right = 0;
  ts.size         = 60.0;
  ts.textcolor    = 0x55555555;
  ts.backcolor    = 0xFFFFFFFF;
  ts.multiline    = 0;

  view_t* clipback = view_get_subview(view_base, "cliplistbck");

  if (clipback)
  {
    tg_text_add(clipback);
    tg_text_set(clipback, "PASTEBOARD", ts);
  }
  else
    zc_log_debug("cliplistbck not found");

  // show texture map for debug

  /* view_t* texmap       = view_new("texmap", ((r2_t){0, 0, 150, 150})); */
  /* texmap->needs_touch  = 0; */
  /* texmap->exclude      = 0; */
  /* texmap->texture.full = 1; */
  /* texmap->layout.right = 0; */
  /* texmap->layout.top   = 0; */

  /* ui_manager_add(texmap); */

  // set glossy effect on header

  /* view_t* header = view_get_subview(view_base, "header"); */
  /* header->texture.blur = 1; */
  /* header->texture.shadow = 1; */

  /* REL(key_cb); // REL 1 */
  /* REL(but_cb); // REL 2 */
}

void ui_add_cursor()
{
  rep_cur                          = view_new("rep_cur", ((r2_t){10, 10, 10, 10}));
  rep_cur->exclude                 = 0;
  rep_cur->layout.background_color = 0xFF000099;
  rep_cur->needs_touch             = 0;
  tg_css_add(rep_cur);
  ui_manager_add_to_top(rep_cur);
}

void ui_update_cursor(r2_t frame)
{
  view_set_frame(rep_cur, frame);
}

void ui_render_without_cursor(uint32_t time)
{
  ui_manager_remove(rep_cur);
  ui_manager_render(time);
  ui_manager_add_to_top(rep_cur);
}

void ui_destroy()
{
  ui_filelist_detach();         // DETACH 0
  ui_song_infos_detach();       // DETACH 1
  ui_visualizer_detach();       // DETACH 2
  ui_filter_bar_detach();       // DETACH 3
  ui_about_popup_detach();      // DETACH 4
  ui_alert_popup_detach();      // DETACH 5
  ui_filter_popup_detach();     // DETACH 6
  ui_play_controls_detach();    // DETACH 8
  ui_decision_popup_detach();   // DETACH 9
  ui_lib_init_popup_detach();   // DETACH 10
  ui_activity_popup_detach();   // DETACH 11
  ui_settings_popup_detach();   // DETACH 12
  ui_song_menu_popup_detach();  // DETACH 13
  ui_inputfield_popup_detach(); // DETACH 14
  ui_popup_switcher_detach();   // DETACH 15

  REL(view_list);

  ui_manager_destroy(); // DESTROY 1

  text_destroy(); // DESTROY 0
}

// key event from base view

void ui_on_key_down(void* userdata, void* data)
{
  ev_t* ev = (ev_t*)data;
  if (ev->keycode == SDLK_SPACE) ui_play_pause();
}

// button down event from descriptor html

void ui_on_button_down(void* userdata, void* data)
{
  char* id = ((view_t*)data)->id;

  if (strcmp(id, "app_close_btn") == 0) wm_close();
  if (strcmp(id, "app_maximize_btn") == 0) wm_toggle_fullscreen();

  // todo sanitize button names

  if (strcmp(id, "aboutbtn") == 0) ui_popup_switcher_toggle("about_popup_page");
  if (strcmp(id, "song_info") == 0) ui_popup_switcher_toggle("messages_popup_page");
  if (strcmp(id, "settingsbtn") == 0) ui_settings_popup_show();
  if (strcmp(id, "closefilterbtn") == 0) ui_popup_switcher_toggle("filters_popup_page");
  if (strcmp(id, "closeeditorbtn") == 0) ui_popup_switcher_toggle("song_editor_popup_page");
  if (strcmp(id, "closesettingsbtn") == 0) ui_popup_switcher_toggle("settings_popup_page");
  if (strcmp(id, "library_popup_close_btn") == 0) ui_popup_switcher_toggle("library_popup_page");
}

void ui_save_screenshot(uint32_t time, char hide_cursor)
{
  if (config_get("lib_path"))
  {
    static int cnt    = 0;
    view_t*    root   = ui_manager_get_root();
    r2_t       frame  = root->frame.local;
    bm_rgba_t* screen = bm_rgba_new(frame.w, frame.h); // REL 0

    // remove cursor for screenshot to remain identical

    if (hide_cursor) ui_render_without_cursor(time);

    ui_compositor_render_to_bmp(screen);

    char*      name    = cstr_new_format(20, "screenshot%.3i.png", cnt++); // REL 1
    char*      path    = path_new_append(config_get("lib_path"), name);    // REL 2
    bm_rgba_t* flipped = bm_rgba_new_flip_y(screen);                       // REL 3

    coder_write_png(path, flipped);

    REL(flipped); // REL 3
    REL(name);    // REL 2
    REL(path);    // REL 1
    REL(screen);  // REL 0

    if (hide_cursor) ui_update_cursor(frame); // full screen cursor to indicate screenshot, next step will reset it
  }
}

#endif

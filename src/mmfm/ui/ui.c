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
#include "filemanager.c"
#include "player.c"
#include "tg_css.c"
#include "tg_text.c"
#include "ui_about_popup.c"
#include "ui_activity_popup.c"
#include "ui_alert_popup.c"
#include "ui_compositor.c"
#include "ui_decision_popup.c"
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
#include "ui_table.c"
#include "ui_visualizer.c"
#include "vh_button.c"
#include "vh_key.c"
#include "view_generator.c"
#include "view_layout.c"
#include "wm_connector.c"
#include "zc_log.c"
#include "zc_number.c"
#include "zc_path.c"
#include "zc_vector.c"

view_t* view_base;
vec_t*  view_list;
view_t* rep_cur; // replay cursor

ui_table_t* filelisttable;
ui_table_t* fileinfotable;
ui_table_t* cliptable;

void ui_on_button_down(void* userdata, void* data);
void ui_on_key_down(void* userdata, void* data);

void on_clipboard_fields_update(ui_table_t* table, vec_t* fields)
{
    zc_log_debug("fields update %s", table->id);
}

void on_clipboard_select(ui_table_t* table, vec_t* selected)
{
    zc_log_debug("select %s", table->id);
    mem_describe(selected, 0);

    map_t* info = selected->data[0];

    vec_t* keys = VNEW();
    map_keys(info, keys);

    vec_t* items = VNEW();
    for (int index = 0; index < keys->length; index++)
    {
	char*  key = keys->data[index];
	map_t* map = MNEW();
	MPUT(map, "key", key);
	MPUT(map, "value", MGET(info, key));
	VADD(items, map);
    }

    ui_table_set_data(fileinfotable, items);

    // REL!!!

    char* path = MGET(info, "path");

    if (path)
    {
	zc_log_debug("SELECTED %s", path);

	if (strstr(path, ".pdf") != NULL)
	{
	    ui_visualizer_show_pdf(path);
	}
    }

    // ui_popup_switcher_toggle("song_popup_page");
}

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

    view_set_frame(view_base, (r2_t){0.0, 0.0, (float) width, (float) height});
    view_layout(view_base);

    ui_manager_add(view_base);

    // attach ui components

    /* ui_cliplist_attach(view_base); // DETACH 0 */
    // ui_meta_view_attach(view_base);
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
    ts.textcolor    = 0xEEEEEEFF;
    ts.backcolor    = 0xFFFFFFFF;
    ts.multiline    = 0;

    vec_t* ffields = VNEW();
    VADDR(ffields, cstr_new_cstring("key"));
    VADDR(ffields, num_new_int(150));
    VADDR(ffields, cstr_new_cstring("value"));
    VADDR(ffields, num_new_int(400));

    view_t* fileinfo       = view_get_subview(view_base, "fileinfotable");
    view_t* fileinfoscroll = view_get_subview(view_base, "fileinfoscroll");
    view_t* fileinfoevt    = view_get_subview(view_base, "fileinfoevt");
    view_t* fileinfohead   = view_get_subview(view_base, "fileinfohead");

    if (fileinfo)
    {
	tg_text_add(fileinfo);
	tg_text_set(fileinfo, "FILE INFO", ts);
    }
    else
	zc_log_debug("fileinfobck not found");

    fileinfotable = ui_table_create(
	"fileinfotable",
	fileinfo,
	fileinfoscroll,
	fileinfoevt,
	fileinfohead,
	ffields,
	on_clipboard_fields_update,
	on_clipboard_select);

    REL(ffields);

    vec_t* fields = VNEW();
    VADDR(fields, cstr_new_cstring("basename"));
    VADDR(fields, num_new_int(100));
    VADDR(fields, cstr_new_cstring("path"));
    VADDR(fields, num_new_int(200));
    VADDR(fields, cstr_new_cstring("size"));
    VADDR(fields, num_new_int(100));
    VADDR(fields, cstr_new_cstring("last_access"));
    VADDR(fields, num_new_int(100));
    VADDR(fields, cstr_new_cstring("last_modification"));
    VADDR(fields, num_new_int(100));
    VADDR(fields, cstr_new_cstring("last_status"));
    VADDR(fields, num_new_int(100));

    view_t* filelist       = view_get_subview(view_base, "filelisttable");
    view_t* filelistscroll = view_get_subview(view_base, "filelistscroll");
    view_t* filelistevt    = view_get_subview(view_base, "filelistevt");
    view_t* filelisthead   = view_get_subview(view_base, "filelisthead");

    if (filelist)
    {
	tg_text_add(filelist);
	tg_text_set(filelist, "FILES", ts);
    }
    else
	zc_log_debug("filelistbck not found");

    filelisttable = ui_table_create(
	"filelisttable",
	filelist,
	filelistscroll,
	filelistevt,
	filelisthead,
	fields,
	on_clipboard_fields_update,
	on_clipboard_select);

    view_t* cliplist       = view_get_subview(view_base, "cliplist");
    view_t* cliplistscroll = view_get_subview(view_base, "cliplistscroll");
    view_t* cliplistevt    = view_get_subview(view_base, "cliplistevt");
    view_t* cliplisthead   = view_get_subview(view_base, "cliplisthead");

    if (cliplist)
    {
	tg_text_add(cliplist);
	tg_text_set(cliplist, "CLIPBOARD", ts);
    }
    else
	zc_log_debug("cliplistbck not found");

    cliptable = ui_table_create(
	"cliptable",
	cliplist,
	cliplistscroll,
	cliplistevt,
	cliplisthead,
	fields,
	on_clipboard_fields_update,
	on_clipboard_select);

    REL(fields);

    map_t* files = MNEW(); // REL 0
    fm_list("/home/milgra/Projects/mmfm", files);
    vec_t* vals = VNEW();
    map_values(files, vals);

    ui_table_set_data(cliptable, vals);

    ui_table_set_data(filelisttable, vals);

    REL(files);
    REL(vals);

    view_t* preview = view_get_subview(view_base, "preview");

    if (preview)
    {
	tg_text_add(preview);
	tg_text_set(preview, "PREVIEW", ts);
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
    /* ui_song_infos_detach();     // DETACH 1 */
    /* ui_visualizer_detach(); // DETACH 2 */
    /* ui_filter_bar_detach(); // DETACH 3 */
    /* ui_about_popup_detach();    // DETACH 4 */
    /* ui_alert_popup_detach();    // DETACH 5 */
    /* ui_filter_popup_detach();   // DETACH 6 */
    /* ui_play_controls_detach(); // DETACH 8 */
    /* ui_decision_popup_detach(); // DETACH 9 */
    /* ui_lib_init_popup_detach(); // DETACH 10 */
    /* ui_activity_popup_detach(); // DETACH 11 */
    // ui_settings_popup_detach();   // DETACH 12
    /* ui_song_menu_popup_detach(); // DETACH 13 */
    /* ui_inputfield_popup_detach(); // DETACH 14 */
    /* ui_popup_switcher_detach();   // DETACH 15 */

    REL(view_list);

    ui_manager_destroy(); // DESTROY 1

    text_destroy(); // DESTROY 0
}

// key event from base view

void ui_on_key_down(void* userdata, void* data)
{
    ev_t* ev = (ev_t*) data;
    if (ev->keycode == SDLK_SPACE) ui_play_pause();
}

// button down event from descriptor html

void ui_on_button_down(void* userdata, void* data)
{
    char* id = ((view_t*) data)->id;

    if (strcmp(id, "app_close_btn") == 0) wm_close();
    if (strcmp(id, "app_maximize_btn") == 0) wm_toggle_fullscreen();

    // todo sanitize button names

    if (strcmp(id, "aboutbtn") == 0) ui_popup_switcher_toggle("about_popup_page");
    if (strcmp(id, "song_info") == 0) ui_popup_switcher_toggle("messages_popup_page");
    // if (strcmp(id, "settingsbtn") == 0) ui_settings_popup_show();
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

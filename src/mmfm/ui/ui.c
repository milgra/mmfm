#ifndef ui_h
#define ui_h

#include "ku_view.c"
#include "ku_window.c"

typedef enum _ui_inputmode ui_inputmode;
enum _ui_inputmode
{
    UI_IM_RENAME
};

void ui_init(int width, int height, float scale, ku_window_t* window);
void ui_destroy();
void ui_add_cursor();
void ui_update_cursor(ku_rect_t frame);
void ui_update_dragger();
void ui_render_without_cursor(uint32_t time, ku_bitmap_t* bm);
void ui_save_screenshot(uint32_t time, char hide_cursor, ku_bitmap_t* bm);
void ui_update_layout(int w, int h);
void ui_describe();
void ui_update_player();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "coder.c"
#include "config.c"
#include "filemanager.c"
#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_type.c"
#include "ku_table.c"
#include "ku_util.c"
#include "ku_wl_connector.c"
#include "map_util.c"
#include "mediaplayer.c"
#include "pdf.c"
#include "tg_css.c"
#include "tg_scaledimg.c"
#include "tg_text.c"
#include "vh_button.c"
#include "vh_cv_body.c"
#include "vh_cv_evnt.c"
#include "vh_cv_scrl.c"
#include "vh_drag.c"
#include "vh_key.c"
#include "vh_textinput.c"
#include "vh_touch.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_number.c"
#include "zc_path.c"
#include "zc_time.c"
#include <limits.h>

struct _ui_t
{
    ku_view_t* view_base;
    ku_view_t* view_drag; // drag overlay
    ku_view_t* view_doc;  // file drag icon
    ku_view_t* cursor;    // replay cursor

    ku_view_t* view_infotf;
    ku_view_t* view_maingap;

    ku_view_t* cliplistbox;
    ku_view_t* fileinfobox;
    ku_view_t* sidebar;

    ku_view_t* exit_btn;
    ku_view_t* full_btn;
    ku_view_t* clip_btn;
    ku_view_t* sidebar_btn;

    ku_view_t* prev_btn;
    ku_view_t* next_btn;
    ku_view_t* play_btn;
    ku_view_t* settings_btn;

    ku_view_t* main_bottom;
    ku_view_t* left_dragger;
    ku_view_t* prev_dragger;

    vec_t* file_list_data;
    vec_t* clip_list_data;
    vec_t* drag_data;

    ku_view_t* visubody;
    ku_view_t* visuvideo;

    ku_view_t* inputarea;
    ku_view_t* inputbck;
    ku_view_t* inputtf;

    ku_table_t* cliptable;
    ku_table_t* filelisttable;
    ku_table_t* fileinfotable;
    ku_table_t* settingstable;
    ku_table_t* contexttable;

    ku_view_t* settingspopupcont;
    ku_view_t* contextpopupcont;

    textstyle_t background_style;
    textstyle_t progress_style;
    textstyle_t inputts;

    MediaState_t* ms;

    int          autoplay;
    char*        current_folder;
    ui_inputmode inputmode;
    ku_view_t*   rowview_for_context_menu;
    map_t*       edited_file;

    ku_window_t* window;
} ui;

int ui_comp_entry(void* left, void* right)
{
    map_t* l = left;
    map_t* r = right;

    char* la = MGET(l, "file/basename");
    char* ra = MGET(r, "file/basename");

    if (la && ra)
    {
	return strcmp(la, ra);
    }
    else
    {
	if (la)
	    return -1;
	else if (ra)
	    return 1;
	else
	    return 0;
    }
}

void ui_show_progress(char* progress)
{
    tg_text_set(ui.view_infotf, progress, ui.progress_style);
}

void ui_on_mp_event(ms_event_t event)
{
    vh_cv_body_set_content_size(ui.visubody, (int) event.rect.x, (int) event.rect.y);
}

void ui_open(char* path)
{
    if (ui.ms)
    {
	mp_close(ui.ms);
	ui.ms = NULL;
    }

    vh_cv_body_set_content_size(ui.visubody, 100, 100);

    if (ui.visuvideo->texture.bitmap != NULL)
    {
	ku_draw_rect(ui.visuvideo->texture.bitmap, 0, 0, 100, 100, 0x00000000, 1);
	ui.visuvideo->texture.changed = 1;
    }

    if (strstr(path, ".pdf") != NULL)
    {
	ku_bitmap_t* pdfbmp = pdf_render(path);

	vh_cv_body_set_content_size(ui.visubody, pdfbmp->w, pdfbmp->h);

	if (ui.visuvideo->texture.bitmap != NULL)
	{
	    ku_draw_insert(ui.visuvideo->texture.bitmap, pdfbmp, 0, 0);
	    ui.visuvideo->texture.changed = 1;
	}

	REL(pdfbmp);
    }
    else
    {
	coder_media_type_t type = coder_get_type(path);

	if (type == CODER_MEDIA_TYPE_VIDEO || type == CODER_MEDIA_TYPE_AUDIO)
	{
	    ui.ms = mp_open(path, ui_on_mp_event);
	}
	else if (type == CODER_MEDIA_TYPE_IMAGE)
	{
	    ku_bitmap_t* image = coder_load_image(path);

	    if (image)
	    {
		vh_cv_body_set_content_size(ui.visubody, image->w, image->h);

		if (ui.visuvideo->texture.bitmap != NULL)
		{
		    ku_draw_insert(ui.visuvideo->texture.bitmap, image, 0, 0);
		    ui.visuvideo->texture.changed = 1;
		}

		REL(image);
	    }
	}
    }
}

void ui_show_info(map_t* info)
{
    if (!MGET(info, "file/mime")) fm_detail(info); // request more info if file not yet detailed

    vec_t* keys = VNEW(); // REL L0
    map_keys(info, keys);
    vec_sort(keys, ((int (*)(void*, void*)) strcmp));

    vec_t* items = VNEW(); // REL L1
    for (int index = 0; index < keys->length; index++)
    {
	char*  key = keys->data[index];
	map_t* map = MNEW();
	MPUT(map, "key", key);
	MPUT(map, "value", MGET(info, key));
	VADDR(items, map);
    }

    REL(keys); // REL L0

    ku_table_set_data(ui.fileinfotable, items);

    ui_show_progress("File info/data loaded");

    REL(items); // REL L1
}

void ui_load_folder(char* folder)
{
    printf("load folder %s\n", folder);
    if (folder) RET(folder); // avoid freeing up if the same pointer is passed to us
    if (ui.current_folder) REL(ui.current_folder);
    if (folder)
    {
	ui.current_folder = folder;

	map_t* files = MNEW(); // REL 0
	zc_time(NULL);
	fm_list(folder, files);
	zc_time("file list");

	vec_reset(ui.file_list_data);
	map_values(files, ui.file_list_data);
	vec_sort(ui.file_list_data, ui_comp_entry);

	ku_table_set_data(ui.filelisttable, ui.file_list_data);
	REL(files);

	ui_show_progress("Directory loaded");
    }
    else printf("EMPTY FOLDER\n");
}

void ui_delete_selected()
{
    if (ui.filelisttable->selected_items->length > 0)
    {
	map_t* file = ui.filelisttable->selected_items->data[0];
	char*  path = MGET(file, "file/path");
	fm_delete(path);
	ui_load_folder(ui.current_folder);
    }
}

void on_files_event(ku_table_event_t event)
{
    if (event.id == KU_TABLE_EVENT_FIELDS_UPDATE)
    {
	zc_log_debug("fields update %s", event.table->id);
    }
    else if (event.id == KU_TABLE_EVENT_SELECT)
    {
	zc_log_debug("select %s", event.table->id);

	map_t* info = event.selected_items->data[0];
	char*  path = MGET(info, "file/path");
	char*  type = MGET(info, "file/type");

	if (strcmp(type, "directory") != 0) ui_open(path);
	ui_show_info(info);
    }
    else if (event.id == KU_TABLE_EVENT_OPEN)
    {
	zc_log_debug("open %s", event.table->id);

	map_t* info = event.selected_items->data[0];

	char* type = MGET(info, "file/type");
	char* path = MGET(info, "file/path");

	if (strcmp(type, "directory") == 0) ui_load_folder(path);
    }
    else if (event.id == KU_TABLE_EVENT_DRAG)
    {
	ku_view_t* docview              = ku_view_new("dragged_view", ((ku_rect_t){.x = 0, .y = 0, .w = 50, .h = 50}));
	char*      imagepath            = cstr_new_format(100, "%s/img/%s", config_get("res_path"), "freebsd.png");
	docview->style.background_image = imagepath;
	tg_css_add(docview);

	ku_view_insert_subview(ui.view_base, docview, ui.view_base->views->length - 2);

	vh_drag_drag(ui.view_drag, docview);
	REL(docview);

	ui.drag_data = event.selected_items;
	ui.view_doc  = docview;
    }
    else if (event.id == KU_TABLE_EVENT_KEY)
    {
	if (event.ev.keycode == XKB_KEY_Down || event.ev.keycode == XKB_KEY_Up)
	{
	    int32_t index = event.selected_index;

	    if (event.ev.keycode == XKB_KEY_Down) index += 1;
	    if (event.ev.keycode == XKB_KEY_Up) index -= 1;

	    ku_table_select(event.table, index);

	    map_t* info = event.selected_items->data[0];
	    char*  path = MGET(info, "file/path");
	    char*  type = MGET(info, "file/type");

	    if (strcmp(type, "directory") != 0) ui_open(path);
	    ui_show_info(info);
	}
	else if (event.ev.keycode == XKB_KEY_Return)
	{
	    map_t* info = event.selected_items->data[0];

	    char* type = MGET(info, "file/type");
	    char* path = MGET(info, "file/path");

	    if (strcmp(type, "directory") == 0) ui_load_folder(path);
	}
    }
    else if (event.id == KU_TABLE_EVENT_CONTEXT)
    {
	if (ui.contextpopupcont->parent == NULL)
	{
	    if (event.rowview)
	    {
		ui.rowview_for_context_menu = event.rowview;

		ku_view_t* contextpopup = ui.contextpopupcont->views->data[0];
		ku_rect_t  iframe       = contextpopup->frame.global;
		iframe.x                = event.ev.x;
		iframe.y                = event.ev.y;
		ku_view_set_frame(contextpopup, iframe);

		ku_view_add_subview(ui.view_base, ui.contextpopupcont);
		// ku_layout(ui.view_base);
	    }
	}
    }
}

void on_clipboard_event(ku_table_event_t event)
{
    if (event.id == KU_TABLE_EVENT_FIELDS_UPDATE)
    {
	zc_log_debug("fields update %s", event.table->id);
    }
    else if (event.id == KU_TABLE_EVENT_SELECT)
    {
	zc_log_debug("select %s", event.table->id);

	map_t* info = event.selected_items->data[0];

	mem_describe(info, 0);

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

	ku_table_set_data(ui.fileinfotable, items);

	char* path = MGET(info, "file/path");
	char* type = MGET(info, "file/type");

	if (strcmp(type, "directory") != 0) ui_open(path);
	ui_show_info(info);
    }
    else if (event.id == KU_TABLE_EVENT_DROP)
    {
	if (ui.drag_data)
	{
	    size_t index = (size_t) event.selected_index;
	    zc_log_debug("DROP %i %i", index, ui.drag_data->length);
	    vec_add_in_vector(ui.clip_list_data, ui.drag_data);
	    ui.drag_data = NULL;
	    ku_table_set_data(ui.cliptable, ui.clip_list_data);

	    if (ui.view_doc)
	    {
		ku_view_remove_from_parent(ui.view_doc);
	    }
	}
    }
}

void on_fileinfo_event(ku_table_event_t event)
{
}

void on_settingslist_event(ku_table_event_t event)
{
}

void on_contextlist_event(ku_table_event_t event)
{
    ku_view_remove_from_parent(ui.contextpopupcont);
    if (event.id == KU_TABLE_EVENT_SELECT)
    {
	if (event.selected_index == 0) // rename
	{
	    if (event.rowview)
	    {
		map_t* info = ui.filelisttable->selected_items->data[0];

		ui.inputmode = UI_IM_RENAME;

		ku_rect_t rframe = ui.rowview_for_context_menu->frame.global;
		ku_rect_t iframe = ui.inputbck->frame.global;
		iframe.x         = rframe.x;
		iframe.y         = rframe.y - 5;
		ku_view_set_frame(ui.inputbck, iframe);

		ku_view_add_subview(ui.view_base, ui.inputarea);

		ku_view_layout(ui.view_base);

		ku_window_activate(ui.window, ui.inputtf);
		vh_textinput_activate(ui.inputtf, 1);

		ui.edited_file = info;

		char* value = MGET(info, "file/basename");
		if (!value) value = "";

		if (value)
		{
		    vh_textinput_set_text(ui.inputtf, value);
		}
	    }
	}
	else if (event.selected_index == 1) // delete
	{
	    ui_delete_selected();
	}
	else if (event.selected_index == 2) // send to cb
	{
	    vec_add_in_vector(ui.clip_list_data, ui.filelisttable->selected_items);
	    ku_table_set_data(ui.cliptable, ui.clip_list_data);
	}
	else if (event.selected_index == 3) // paste using copy
	{
	}
	else if (event.selected_index == 4) // paste using move
	{
	}
	else if (event.selected_index == 5) // reset clipboard
	{
	    vec_reset(ui.clip_list_data);
	    ku_table_set_data(ui.cliptable, ui.clip_list_data);
	}
    }
}

void ui_cancel_input()
{
    ku_view_remove_subview(ui.view_base, ui.inputarea);
}

void ui_on_touch(vh_touch_event_t event)
{
    if (strcmp(event.view->id, "inputarea") == 0)
    {
	ui_cancel_input();
    }
    else if (strcmp(event.view->id, "contextpopupcont") == 0)
    {
	ku_view_remove_from_parent(ui.contextpopupcont);
    }
}

void ui_toggle_pause()
{
    ui.autoplay = 1 - ui.autoplay;

    vh_button_set_state(ui.play_btn, ui.autoplay ? VH_BUTTON_UP : VH_BUTTON_DOWN);

    if (ui.ms)
    {
	if (ui.autoplay == 0 && !ui.ms->paused)
	{
	    mp_pause(ui.ms);
	}

	if (ui.autoplay == 1 && ui.ms->paused)
	{
	    mp_play(ui.ms);
	}
    }
}

void ui_on_key_down(vh_key_event_t event)
{
    if (event.ev.keycode == XKB_KEY_space) ui_toggle_pause();
    if (event.ev.keycode == XKB_KEY_Delete) ui_delete_selected();
    if (event.ev.keycode == XKB_KEY_c && event.ev.ctrl_down)
    {
	vec_add_in_vector(ui.clip_list_data, ui.filelisttable->selected_items);
	ku_table_set_data(ui.cliptable, ui.clip_list_data);
    }
    if (event.ev.keycode == XKB_KEY_v && event.ev.ctrl_down)
    {
	// show paste menu
    }
}

void ui_on_btn_event(vh_button_event_t event)
{
    ku_view_t* btnview = event.view;

    printf("BUTTON EVENT\n");

    if (btnview == ui.exit_btn) ku_wayland_exit();
    if (btnview == ui.full_btn) ku_wayland_toggle_fullscreen(ui.window);

    if (btnview == ui.clip_btn)
    {
	ku_view_t* top = ku_view_get_subview(ui.view_base, "top_container");
	if (ui.fileinfobox->parent) ku_view_remove_from_parent(ui.fileinfobox);

	if (ui.cliplistbox->parent)
	{
	    ku_view_remove_from_parent(ui.cliplistbox);
	}
	else
	{
	    ku_view_add_subview(top, ui.cliplistbox);
	}
	ku_view_layout(top);
    }
    if (btnview == ui.sidebar_btn)
    {
	ku_view_t* top = ku_view_get_subview(ui.view_base, "top_container");

	if (ui.sidebar->parent)
	{
	    ku_view_remove_from_parent(ui.sidebar);
	    config_set_bool("sidebar_visible", 0);
	}
	else
	{
	    ku_view_add_subview(top, ui.sidebar);
	    config_set_bool("sidebar_visible", 1);
	}

	config_write(config_get("cfg_path"));
	ku_view_layout(top);
    }
    if (btnview == ui.prev_btn)
    {
	int32_t index = ui.filelisttable->selected_index - 1;

	ku_table_select(ui.filelisttable, index);

	map_t* info = ui.filelisttable->selected_items->data[0];
	char*  path = MGET(info, "file/path");
	char*  type = MGET(info, "file/type");

	if (strcmp(type, "directory") != 0) ui_open(path);
	ui_show_info(info);
    }
    if (btnview == ui.next_btn)
    {
	int32_t index = ui.filelisttable->selected_index + 1;

	ku_table_select(ui.filelisttable, index);

	map_t* info = ui.filelisttable->selected_items->data[0];
	char*  path = MGET(info, "file/path");
	char*  type = MGET(info, "file/type");

	if (strcmp(type, "directory") != 0) ui_open(path);
	ui_show_info(info);
    }
    if (btnview == ui.play_btn)
    {
	if (event.vh->state == VH_BUTTON_DOWN)
	{
	    ui.autoplay = 0;
	    if (ui.ms) mp_pause(ui.ms);
	}
	else
	{
	    ui.autoplay = 1;
	    if (ui.ms) mp_play(ui.ms);
	}
    }
    if (btnview == ui.settings_btn)
    {
	if (!ui.settingspopupcont->parent)
	{
	    ku_view_add_subview(ui.view_base, ui.settingspopupcont);
	    ku_view_layout(ui.view_base);
	}
    }
    if (strcmp(event.view->id, "settingsclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.settingspopupcont);
    }
}

void ui_on_touch_event(vh_touch_event_t event)
{
    if (event.view == ui.left_dragger)
    {
	vh_drag_drag(ui.view_drag, ui.left_dragger);
    }
    if (event.view == ui.prev_dragger)
    {
	vh_drag_drag(ui.view_drag, ui.prev_dragger);
    }
}

void ui_on_text_event(vh_textinput_event_t event)
{
    if (strcmp(event.view->id, "inputtf") == 0)
    {
	if (event.id == VH_TEXTINPUT_DEACTIVATE) ui_cancel_input();
	if (event.id == VH_TEXTINPUT_RETURN)
	{
	    ui_cancel_input();

	    if (ui.edited_file)
	    {

		char* parent = MGET(ui.edited_file, "file/parent");

		char* oldpath = MGET(ui.edited_file, "file/path");
		char* newpath = path_new_append(parent, event.text);

		fm_rename(oldpath, newpath, NULL);

		REL(newpath);

		ui_load_folder(ui.current_folder);
	    }
	}
    }
}

void ui_on_drag(vh_drag_event_t event)
{
    if (event.dragged_view == ui.left_dragger)
    {
	ku_view_t* cbox    = ku_view_get_subview(ui.view_base, "cliplistbox");
	ku_view_t* fbox    = ku_view_get_subview(ui.view_base, "fileinfobox");
	ku_view_t* lft     = ku_view_get_subview(ui.view_base, "left_container");
	ku_view_t* top     = ku_view_get_subview(ui.view_base, "top_container");
	cbox->style.height = lft->frame.global.h - event.dragged_view->frame.global.y + 28.0;
	fbox->style.width  = top->frame.global.w - event.dragged_view->frame.global.x - 9;
	zc_log_debug("height %i width %i", cbox->style.height, fbox->style.width);
	ku_view_layout(top);
    }
}

void ui_add_cursor()
{
    ui.cursor                         = ku_view_new("ui.cursor", ((ku_rect_t){10, 10, 10, 10}));
    ui.cursor->exclude                = 0;
    ui.cursor->style.background_color = 0xFF000099;
    ui.cursor->needs_touch            = 0;
    tg_css_add(ui.cursor);
    /* ku_window_add_to_top(ui.window,ui.cursor); */
}

void ui_update_cursor(ku_rect_t frame)
{
    ku_view_set_frame(ui.cursor, frame);
}

void ui_render_without_cursor(uint32_t time, ku_bitmap_t* bm)
{
    /* ku_window_remove(ui.window,ui.cursor); */
    /* ku_window_render(ui.window,time, bm); */
    /* ku_window_add_to_top(ui.window,ui.cursor); */
}

void ui_save_screenshot(uint32_t time, char hide_cursor, ku_bitmap_t* bm)
{
    /* if (config_get("lib_path")) */
    /* { */
    /* 	static int cnt    = 0; */
    /* 	ku_view_t*    root   = ku_window_get_root(ui.window); */
    /* 	ku_rect_t       frame  = root->frame.local; */
    /* 	ku_bitmap_t* screen = ku_bitmap_new(frame.w, frame.h); // REL 0 */

    /* 	// remove cursor for screenshot to remain identical */

    /* 	if (hide_cursor) ui_render_without_cursor(time, bm); */

    /* 	ui_compositor_render_to_bmp(screen); */

    /* 	char*      name    = cstr_new_format(20, "screenshot%.3i.png", cnt++); // REL 1 */
    /* 	char*      path    = path_new_append(config_get("lib_path"), name);    // REL 2 */
    /* 	ku_bitmap_t* flipped = ku_bitmap_new_flip_y(screen);                       // REL 3 */

    /* 	coder_write_png(path, flipped); */

    /* 	REL(flipped); // REL 3 */
    /* 	REL(name);    // REL 2 */
    /* 	REL(path);    // REL 1 */
    /* 	REL(screen);  // REL 0 */

    /* 	if (hide_cursor) ui_update_cursor(frame); // full screen cursor to indicate screenshot, next step will reset it */
    /* } */
}

int ui_comp_text(void* left, void* right)
{
    char* la = left;
    char* ra = right;

    return strcmp(la, ra);
}

void ui_init(int width, int height, float scale, ku_window_t* window)
{
    ku_text_init(); // DESTROY 0

    ui.window   = window;
    ui.autoplay = 1;

    /* generate views from descriptors */

    vec_t* view_list = VNEW();

    ku_gen_html_parse(config_get("html_path"), view_list);
    ku_gen_css_apply(view_list, config_get("css_path"), config_get("res_path"), 1.0);
    ku_gen_type_apply(view_list, NULL, NULL); // TODO use btn and slider event

    ui.view_base = RET(vec_head(view_list));

    REL(view_list);

    /* initial layout of views */

    ku_view_set_frame(ui.view_base, (ku_rect_t){0.0, 0.0, (float) width, (float) height});
    ku_window_add(ui.window, ui.view_base);

    /* listen for keys and shortcuts */

    vh_key_add(ui.view_base, ui_on_key_down);
    ui.view_base->needs_key = 1;
    ku_window_activate(ui.window, ui.view_base);

    /* setup drag layer */

    ui.view_drag = ku_view_get_subview(ui.view_base, "draglayer");
    vh_drag_attach(ui.view_drag, ui_on_drag);

    /* setup visualizer */

    ui.visuvideo = ku_view_get_subview(ui.view_base, "previewcont");
    ui.visubody  = ku_view_get_subview(ui.view_base, "previewbody");

    /* tables */

    ku_view_t* filelist = ku_view_get_subview(ui.view_base, "filelisttable");

    textstyle_t ts = ku_util_gen_textstyle(filelist);

    ts.align        = TA_CENTER;
    ts.margin_right = 0;
    ts.size         = 60.0;
    ts.textcolor    = 0x353535FF;
    ts.backcolor    = 0x252525FF;
    ts.multiline    = 0;

    ui.background_style = ts;

    /* preview block */

    ku_view_t* preview     = ku_view_get_subview(ui.view_base, "preview");
    ku_view_t* previewbody = ku_view_get_subview(ui.view_base, "previewbody");
    ku_view_t* previewscrl = ku_view_get_subview(ui.view_base, "previewscrl");
    ku_view_t* previewevnt = ku_view_get_subview(ui.view_base, "previewevnt");
    ku_view_t* previewcont = ku_view_get_subview(ui.view_base, "previewcont");

    if (preview)
    {
	tg_text_add(preview);
	tg_text_set(preview, "PREVIEW", ts);

	tg_scaledimg_add(previewcont, 30, 30);
	ku_view_set_frame(previewcont, (ku_rect_t){0, 0, 30, 30});
	previewcont->style.margin = INT_MAX;

	vh_cv_body_attach(previewbody, NULL);
	vh_cv_scrl_attach(previewscrl, previewbody, NULL);
	vh_cv_evnt_attach(previewevnt, previewbody, previewscrl, NULL);
    }
    else
	zc_log_debug("preview not found");

    /* file info table */

    vec_t* fields = VNEW();
    VADDR(fields, cstr_new_cstring("key"));
    VADDR(fields, num_new_int(150));
    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(400));

    ku_view_t* fileinfo       = ku_view_get_subview(ui.view_base, "fileinfotable");
    ku_view_t* fileinfoscroll = ku_view_get_subview(ui.view_base, "fileinfoscroll");
    ku_view_t* fileinfoevt    = ku_view_get_subview(ui.view_base, "fileinfoevt");
    ku_view_t* fileinfohead   = ku_view_get_subview(ui.view_base, "fileinfohead");

    if (fileinfo)
    {
	tg_text_add(fileinfo);
	tg_text_set(fileinfo, "FILE INFO", ts);

	ui.fileinfotable = ku_table_create(
	    "fileinfotable",
	    fileinfo,
	    fileinfoscroll,
	    fileinfoevt,
	    fileinfohead,
	    fields,
	    on_fileinfo_event);
    }
    else
	zc_log_debug("fileinfobck not found");

    REL(fields);

    /* // files table */

    fields = VNEW();
    VADDR(fields, cstr_new_cstring("file/basename"));
    VADDR(fields, num_new_int(400));
    /* VADDR(fields, cstr_new_cstring("file/mime")); */
    /* VADDR(fields, num_new_int(200)); */
    /* VADDR(fields, cstr_new_cstring("file/path")); */
    /* VADDR(fields, num_new_int(200)); */
    VADDR(fields, cstr_new_cstring("file/size"));
    VADDR(fields, num_new_int(120));
    VADDR(fields, cstr_new_cstring("file/last_access"));
    VADDR(fields, num_new_int(180));
    VADDR(fields, cstr_new_cstring("file/last_modification"));
    VADDR(fields, num_new_int(180));
    VADDR(fields, cstr_new_cstring("file/last_status"));
    VADDR(fields, num_new_int(180));

    // ku_view_t* filelist       = ku_view_get_subview(ui.view_base, "filelisttable");
    ku_view_t* filelistscroll = ku_view_get_subview(ui.view_base, "filelistscroll");
    ku_view_t* filelistevt    = ku_view_get_subview(ui.view_base, "filelistevt");
    ku_view_t* filelisthead   = ku_view_get_subview(ui.view_base, "filelisthead");

    if (filelist)
    {
	tg_text_add(filelist);
	tg_text_set(filelist, "FILES", ts);
    }
    else
	zc_log_debug("filelistbck not found");

    ui.filelisttable = ku_table_create(
	"filelisttable",
	filelist,
	filelistscroll,
	filelistevt,
	filelisthead,
	fields,
	on_files_event);

    ui.file_list_data = VNEW(); // REL S0
    ku_table_set_data(ui.filelisttable, ui.file_list_data);

    ku_window_activate(ui.window, filelistevt);

    // clipboard table

    ku_view_t* cliplist       = ku_view_get_subview(ui.view_base, "cliplist");
    ku_view_t* cliplistscroll = ku_view_get_subview(ui.view_base, "cliplistscroll");
    ku_view_t* cliplistevt    = ku_view_get_subview(ui.view_base, "cliplistevt");
    ku_view_t* cliplisthead   = ku_view_get_subview(ui.view_base, "cliplisthead");

    if (cliplist)
    {
	tg_text_add(cliplist);
	tg_text_set(cliplist, "CLIPBOARD", ts);

	ui.cliptable = ku_table_create(
	    "cliptable",
	    cliplist,
	    cliplistscroll,
	    cliplistevt,
	    cliplisthead,
	    fields,
	    on_clipboard_event);

	ui.clip_list_data = VNEW();
	ku_table_set_data(ui.cliptable, ui.clip_list_data);
    }
    else
	zc_log_debug("cliplistbck not found");

    REL(fields);

    /* close button */

    ui.exit_btn = ku_view_get_subview(ui.view_base, "app_close_btn");
    vh_button_add(ui.exit_btn, VH_BUTTON_NORMAL, ui_on_btn_event);

    ui.full_btn = ku_view_get_subview(ui.view_base, "app_maximize_btn");
    vh_button_add(ui.full_btn, VH_BUTTON_NORMAL, ui_on_btn_event);

    /* ui.clip_btn = ku_view_get_subview(ui.view_base, "show_clipboard_btn"); */
    /* vh_button_add(ui.clip_btn, VH_BUTTON_NORMAL, btn_cb); */

    ui.sidebar_btn = ku_view_get_subview(ui.view_base, "show_properties_btn");
    vh_button_add(ui.sidebar_btn, VH_BUTTON_NORMAL, ui_on_btn_event);

    ui.play_btn = ku_view_get_subview(ui.view_base, "playbtn");
    vh_button_add(ui.play_btn, VH_BUTTON_TOGGLE, ui_on_btn_event);

    ui.next_btn = ku_view_get_subview(ui.view_base, "nextbtn");
    vh_button_add(ui.next_btn, VH_BUTTON_NORMAL, ui_on_btn_event);

    ui.prev_btn = ku_view_get_subview(ui.view_base, "prevbtn");
    vh_button_add(ui.prev_btn, VH_BUTTON_NORMAL, ui_on_btn_event);

    ui.settings_btn = ku_view_get_subview(ui.view_base, "settingsbtn");
    vh_button_add(ui.settings_btn, VH_BUTTON_NORMAL, ui_on_btn_event);

    ku_view_t* settingsclosebtn = ku_view_get_subview(ui.view_base, "settingsclosebtn");
    vh_button_add(settingsclosebtn, VH_BUTTON_NORMAL, ui_on_btn_event);

    // get main bottom for layout change

    ui.main_bottom  = ku_view_get_subview(ui.view_base, "main_bottom");
    ui.left_dragger = ku_view_get_subview(ui.view_base, "left_dragger");

    if (ui.left_dragger)
    {
	ui.left_dragger->blocks_touch = 1;
	vh_touch_add(ui.left_dragger, ui_on_touch_event);
    }

    /* settings list */

    ku_view_t* settingspopupcont = ku_view_get_subview(ui.view_base, "settingspopupcont");
    ku_view_t* settingspopup     = ku_view_get_subview(ui.view_base, "settingspopup");
    ku_view_t* settingslist      = ku_view_get_subview(ui.view_base, "settingstable");
    ku_view_t* settingslistevt   = ku_view_get_subview(ui.view_base, "settingslistevt");

    settingspopup->blocks_touch  = 1;
    settingspopup->blocks_scroll = 1;

    ui.settingspopupcont = RET(settingspopupcont);

    fields = VNEW();

    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(510));

    ui.settingstable = ku_table_create(
	"settingstable",
	settingslist,
	NULL,
	settingslistevt,
	NULL,
	fields,
	on_settingslist_event);

    REL(fields);

    vec_t* items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "MultiMedia File Manager v%s beta by Milan Toth", MMFM_VERSION)}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Free and Open Source Software")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Donate on Paypal")}));

    ku_table_set_data(ui.settingstable, items);
    REL(items);

    ku_view_remove_from_parent(ui.settingspopupcont);

    /* context list */

    ku_view_t* contextpopupcont = ku_view_get_subview(ui.view_base, "contextpopupcont");
    ku_view_t* contextpopup     = ku_view_get_subview(ui.view_base, "contextpopup");
    ku_view_t* contextlist      = ku_view_get_subview(ui.view_base, "contexttable");
    ku_view_t* contextlistevt   = ku_view_get_subview(ui.view_base, "contextlistevt");

    contextpopup->blocks_touch  = 1;
    contextpopup->blocks_scroll = 1;

    ui.contextpopupcont = RET(contextpopupcont);

    fields = VNEW();

    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(200));

    ui.contexttable = ku_table_create(
	"contexttable",
	contextlist,
	NULL,
	contextlistevt,
	NULL,
	fields,
	on_contextlist_event);

    REL(fields);

    items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Rename")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Delete")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Send to clipboard")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Paste using copy")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Paste using move")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Reset clipboard")}));

    ku_table_set_data(ui.contexttable, items);
    REL(items);

    vh_touch_add(ui.contextpopupcont, ui_on_touch);

    ku_view_remove_from_parent(ui.contextpopupcont);

    // info textfield

    ui.view_infotf = ku_view_get_subview(ui.view_base, "infotf");
    tg_text_add(ui.view_infotf);

    ui.progress_style = ku_util_gen_textstyle(ui.view_infotf);
    ui_show_progress("Directory loaded");

    /* input popup */

    ui.inputarea = RET(ku_view_get_subview(ui.view_base, "inputarea"));
    ui.inputbck  = ku_view_get_subview(ui.view_base, "inputbck");
    ui.inputtf   = ku_view_get_subview(ui.view_base, "inputtf");
    ui.inputts   = ku_util_gen_textstyle(ui.inputtf);

    ui.inputbck->blocks_touch   = 1;
    ui.inputarea->blocks_touch  = 1;
    ui.inputarea->blocks_scroll = 1;

    vh_textinput_add(ui.inputtf, "Generic input", "", ui.inputts, ui_on_text_event);

    vh_touch_add(ui.inputarea, ui_on_touch);

    ku_view_remove_from_parent(ui.inputarea);

    // main gap

    ui.view_maingap = ku_view_get_subview(ui.view_base, "main_gap");

    ui.cliplistbox = RET(ku_view_get_subview(ui.view_base, "cliplistbox"));
    ui.fileinfobox = RET(ku_view_get_subview(ui.view_base, "fileinfobox"));

    ui.sidebar = RET(ku_view_get_subview(ui.view_base, "sidebar"));

    if (config_get_bool("sidebar_visible") == 0) ku_view_remove_from_parent(ui.sidebar);

    // fill up files table

    ui_load_folder(config_get("top_path"));

    // show texture map for debug

    /* ku_view_t* texmap       = ku_view_new("texmap", ((ku_rect_t){0, 0, 150, 150})); */
    /* texmap->needs_touch  = 0; */
    /* texmap->exclude      = 0; */
    /* texmap->texture.full = 1; */
    /* texmap->style.right  = 0; */
    /* texmap->style.top    = 0; */

    /* ku_window_add(ui.window,texmap); */
}

void ui_destroy()
{
    if (ui.ms)
    {
	mp_close(ui.ms);
	ui.ms = NULL;
    }

    ku_window_remove(ui.window, ui.view_base);

    REL(ui.cliplistbox);
    REL(ui.fileinfobox);

    REL(ui.view_base);
    REL(ui.fileinfotable);
    REL(ui.filelisttable);
    REL(ui.cliptable);
    REL(ui.settingstable);
    REL(ui.contexttable);

    REL(ui.file_list_data);
    REL(ui.clip_list_data);

    REL(ui.settingspopupcont);
    REL(ui.contextpopupcont);
    REL(ui.sidebar);

    REL(ui.inputarea);

    ku_text_destroy(); // DESTROY 0
}

void ui_update_layout(int w, int h)
{
    if (w > h)
    {
	ui.main_bottom->style.flexdir = FD_ROW;

	if (ui.view_maingap)
	{
	    ui.view_maingap->style.w_per = 0;
	    ui.view_maingap->style.width = 5;

	    ui.view_maingap->style.h_per  = 1;
	    ui.view_maingap->style.height = 0;
	}
    }
    else
    {
	ui.main_bottom->style.flexdir = FD_COL;

	if (ui.view_maingap)
	{
	    ui.view_maingap->style.h_per  = 0;
	    ui.view_maingap->style.height = 5;

	    ui.view_maingap->style.w_per = 1;
	    ui.view_maingap->style.width = 0;
	}
    }
    /* ku_layout(ui.view_base); */
    /* view_describe(ui.view_base, 0); */
}

void ui_update_dragger()
{
    /* ku_view_t* filelist = ku_view_get_subview(ui.view_base, "filelisttable"); */
    /* ku_rect_t    df       = ui.left_dragger->frame.local; */
    /* df.x             = filelist->frame.global.x + filelist->frame.global.w; */
    /* df.y             = filelist->frame.global.y + filelist->frame.global.h; */
    /* ku_view_set_frame(ui.left_dragger, df); */
}

void ui_describe()
{
    ku_view_describe(ui.view_base, 0);
}

void ui_update_player()
{
    if (ui.ms)
    {
	double rem = 0.01;
	mp_video_refresh(ui.ms, &rem, ui.visuvideo->texture.bitmap);
	// mp_audio_refresh(ui.ms, ui.visuvideo->texture.bitmap, ui.visuvideo->texture.bitmap);

	if (ui.autoplay == 0 && !ui.ms->paused)
	{
	    double time = roundf(mp_get_master_clock(ui.ms) * 10.0) / 10.0;

	    if (time > 0.1) mp_pause(ui.ms);
	}

	ui.visuvideo->texture.changed = 1;
    }
}

#endif

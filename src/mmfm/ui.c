#ifndef ui_h
#define ui_h

#include "ku_view.c"
#include "ku_window.c"

typedef enum _ui_inputmode ui_inputmode;
enum _ui_inputmode
{
    UI_IM_RENAME
};

typedef enum _ui_media_type ui_media_type;
enum _ui_media_type
{
    UI_MT_STREAM,
    UI_MT_DOCUMENT,
};

void ui_init(int width, int height, float scale, ku_window_t* window);
void ui_destroy();
void ui_add_cursor();
void ui_update_cursor(ku_rect_t frame);
void ui_update_layout(int w, int h);
void ui_update_player();
void ui_load_folder(char* folder);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "coder.c"
#include "config.c"
#include "filemanager.c"
#include "ku_connector_wayland.c"
#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_type.c"
#include "ku_gl.c"
#include "ku_table.c"
#include "ku_util.c"
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
#include "vh_tbl_body.c"
#include "vh_tbl_scrl.c"
#include "vh_textinput.c"
#include "vh_touch.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_map_ext.c"
#include "zc_number.c"
#include "zc_path.c"
#include "zc_time.c"
#include <limits.h>

struct _ui_t
{
    ku_window_t* window; // window for this ui

    ku_view_t* basev;       // base view
    ku_view_t* dragv;       // drag overlay
    ku_view_t* cursorv;     // replay cursor
    ku_view_t* draggedv;    // file drag icon
    ku_view_t* cliptablev;  // clipboard table
    ku_view_t* infotablev;  // file info table
    ku_view_t* prevbodyv;   // preview boody
    ku_view_t* prevcontv;   // preview container
    ku_view_t* inputbckv;   // input textfield background for positioning
    ku_view_t* mainbottomv; // main bottom container

    ku_view_t* timelv;   // time label
    ku_view_t* statuslv; // status label
    ku_view_t* pathtv;   // path text view
    ku_view_t* inputtv;  // input text view

    ku_view_t* settingscv; // settings popup container view
    ku_view_t* contextcv;  // context popup container view
    ku_view_t* inputcv;    // input popup container view
    ku_view_t* okaycv;     // okay popup container view

    ku_view_t* seekbarv;
    ku_view_t* minusbtnv;
    ku_view_t* plusbtnv;
    ku_view_t* playbtnv;
    ku_view_t* prevbtnv;
    ku_view_t* nextbtnv;

    ku_table_t* cliptable;
    ku_table_t* filetable;
    ku_table_t* infotable;
    ku_table_t* contexttable;
    ku_table_t* settingstable;

    vec_t* focusable_filelist; // focusable views in file list only mode
    vec_t* focusable_infolist; // focusable views in file + info list mode
    vec_t* focusable_cliplist; // focusable views in file + clipboard list mode

    /* text editing */

    ui_inputmode inputmode;

    /* data containers */

    vec_t* filedatav; // files table data
    vec_t* clipdatav; // clipboard table data
    vec_t* dragdatav; // dragged files data

    /* file browser state */

    char*      current_folder;
    map_t*     folder_history;
    ku_view_t* rowview_for_context_menu; // TODO do we need this?

    /* media state */

    MediaState_t*      media_state;
    ui_media_type      media_type;  // media type of ui
    coder_media_type_t coder_type;  // media type of stream TODO these two should be merged
    int                play_state;  // play or pause selected last
    float              last_update; // last update of time label

    int   pdf_page_count;
    int   pdf_page;
    char* pdf_path;
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

void ui_show_pdf_page(int page)
{
    if (page == ui.pdf_page_count) page--;
    if (page == -1) page++;

    ui.pdf_page = page;

    ku_bitmap_t* pdfbmp = pdf_render(ui.pdf_path, ui.pdf_page);

    char timebuff[32];
    snprintf(timebuff, 32, "Page %i / %i", ui.pdf_page + 1, ui.pdf_page_count);

    tg_text_set1(ui.timelv, timebuff);

    vh_cv_body_set_content_size(ui.prevbodyv, pdfbmp->w, pdfbmp->h);

    if (ui.prevcontv->texture.bitmap != NULL)
    {
	ku_draw_insert(ui.prevcontv->texture.bitmap, pdfbmp, 0, 0);
	ku_view_upload_texture(ui.prevcontv);
    }

    vh_slider_set(ui.seekbarv, (float) (ui.pdf_page + 1) / (float) ui.pdf_page_count);

    REL(pdfbmp);
}

void ui_show_progress(char* progress)
{
    tg_text_set1(ui.statuslv, progress);
}

void ui_on_mp_event(ms_event_t event)
{
    printf("ON MP EVENT %f %f\n", event.rect.x, event.rect.y);
    vh_cv_body_set_content_size(ui.prevbodyv, (int) event.rect.x, (int) event.rect.y);
}

void ui_open(char* path)
{
    if (ui.media_state)
    {
	mp_close(ui.media_state);
	ui.media_state = NULL;
    }

    vh_cv_body_set_content_size(ui.prevbodyv, 100, 100);

    if (ui.prevcontv->texture.bitmap != NULL)
    {
	ku_draw_rect(ui.prevcontv->texture.bitmap, 0, 0, 100, 100, 0x00000000, 1);
	ui.prevcontv->texture.changed = 1;
    }

    if (strstr(path, ".pdf") != NULL)
    {
	if (ui.pdf_path) REL(ui.pdf_path);
	ui.pdf_path       = RET(path);
	ui.pdf_page_count = pdf_count(path);
	ui.media_type     = UI_MT_DOCUMENT;
	ui_show_pdf_page(0);

	vh_button_disable(ui.playbtnv);
	vh_button_enable(ui.nextbtnv);
	vh_button_enable(ui.prevbtnv);
	vh_button_enable(ui.plusbtnv);
	vh_button_enable(ui.minusbtnv);
	vh_slider_enable(ui.seekbarv);
    }
    else
    {
	coder_media_type_t type = coder_get_type(path);

	ui.coder_type = type;

	if (type == CODER_MEDIA_TYPE_VIDEO || type == CODER_MEDIA_TYPE_AUDIO)
	{
	    vh_button_enable(ui.playbtnv);
	    vh_button_disable(ui.prevbtnv);
	    vh_button_disable(ui.nextbtnv);
	    vh_button_enable(ui.plusbtnv);
	    vh_button_enable(ui.minusbtnv);
	    vh_slider_enable(ui.seekbarv);

	    ui.media_state = mp_open(path, ui_on_mp_event);
	}
	else if (type == CODER_MEDIA_TYPE_IMAGE)
	{
	    ku_bitmap_t* image = coder_load_image(path);

	    if (image)
	    {
		vh_cv_body_set_content_size(ui.prevbodyv, image->w, image->h);

		if (ui.prevcontv->texture.bitmap != NULL)
		{
		    ku_draw_insert(ui.prevcontv->texture.bitmap, image, 0, 0);
		    ui.prevcontv->texture.changed = 1;
		}

		REL(image);
	    }
	    vh_button_disable(ui.playbtnv);
	    vh_button_disable(ui.prevbtnv);
	    vh_button_disable(ui.nextbtnv);
	    vh_button_enable(ui.plusbtnv);
	    vh_button_enable(ui.minusbtnv);
	    vh_slider_disable(ui.seekbarv);
	    tg_text_set1(ui.timelv, "");
	}
	else
	{
	    vh_button_disable(ui.playbtnv);
	    vh_button_disable(ui.nextbtnv);
	    vh_button_disable(ui.prevbtnv);
	    vh_button_disable(ui.plusbtnv);
	    vh_button_disable(ui.minusbtnv);
	    tg_text_set1(ui.timelv, "");
	    vh_slider_disable(ui.seekbarv);
	}

	ui.media_type = UI_MT_STREAM;
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

    ku_table_set_data(ui.infotable, items);

    ui_show_progress("File info/data loaded");

    REL(items); // REL L1
}

void ui_load_folder(char* folder)
{
    if (folder)
    {
	int prev_selected = ui.filetable->selected_index;

	if (folder != ui.current_folder) prev_selected = 0; // prev selected should stay after delete

	if (ui.current_folder)
	{
	    // store folder as last visited
	    MPUT(ui.folder_history, ui.current_folder, folder);
	}

	if (ui.current_folder) REL(ui.current_folder);

	ui.current_folder = path_new_normalize1(folder);
	vh_textinput_set_text(ui.pathtv, ui.current_folder);

	map_t* files = MNEW(); // REL 0
	zc_time(NULL);
	fm_list(ui.current_folder, files);
	zc_time("file list");

	vec_reset(ui.filedatav);
	map_values(files, ui.filedatav);
	vec_sort(ui.filedatav, ui_comp_entry);

	ku_table_set_data(ui.filetable, ui.filedatav);
	ku_table_select(ui.filetable, prev_selected);
	REL(files);

	/* jump to item if there is a last visited item */
	char* last_visited = MGET(ui.folder_history, ui.current_folder);
	if (last_visited)
	{
	    int index = 0;
	    int found = 0;
	    for (index = 0; index < ui.filedatav->length; index++)
	    {
		map_t* info = ui.filedatav->data[index];
		char*  path = MGET(info, "file/path");
		if (strcmp(path, last_visited) == 0)
		{
		    found = 1;
		    break;
		}
	    }
	    if (found) ku_table_select(ui.filetable, index);
	}
	else

	    ui_show_progress("Directory loaded");
    }
    else printf("EMPTY FOLDER\n");
}

void ui_delete_selected()
{
    if (ui.filetable->selected_items->length > 0)
    {
	if (ui.okaycv->parent == NULL)
	{
	    ku_view_add_subview(ui.basev, ui.okaycv);
	    ku_view_layout(ui.basev);
	}
    }
}

void on_table_event(ku_table_event_t event)
{
    if (event.id == KU_TABLE_EVENT_KEY_DOWN)
    {
	if (event.ev.keycode == XKB_KEY_Down || event.ev.keycode == XKB_KEY_Up)
	{
	    int32_t index = event.selected_index;

	    if (event.ev.keycode == XKB_KEY_Down) index += 1;
	    if (event.ev.keycode == XKB_KEY_Up) index -= 1;

	    ku_table_select(event.table, index);

	    if (event.table == ui.filetable || event.table == ui.cliptable)
	    {
		if (event.ev.repeat == 0 || index == event.table->items->length - 1)
		{
		    map_t* info = event.selected_items->data[0];
		    char*  path = MGET(info, "file/path");
		    char*  type = MGET(info, "file/type");

		    if (strcmp(type, "directory") != 0) ui_open(path);
		    ui_show_info(info);

		    if (index < ui.filedatav->length)
		    {
			ku_view_t*     filetable = ku_view_get_subview(ui.basev, "filetablebody");
			vh_tbl_body_t* vh        = filetable->handler_data;
			int            rel_index = index - vh->head_index;
			ku_view_t*     sel_item  = vh->items->data[rel_index];

			ui.rowview_for_context_menu = sel_item;
		    }
		}
	    }
	}
	else if (event.ev.keycode == XKB_KEY_Return)
	{
	    if (event.table == ui.filetable || event.table == ui.cliptable)
	    {
		map_t* info = event.selected_items->data[0];

		char* type = MGET(info, "file/type");
		char* path = MGET(info, "file/path");

		if (strcmp(type, "directory") == 0) ui_load_folder(path);
	    }
	}
    }

    if (event.table == ui.filetable)
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
	    ku_view_t* docview = ku_view_new("dragged_view", ((ku_rect_t){.x = 0, .y = 0, .w = 50, .h = 50}));
	    /* char*      imagepath            = cstr_new_format(100, "%s/img/%s", config_get("res_path"), "freebsd.png"); */
	    docview->style.background_color = 0x00FF00FF;
	    tg_css_add(docview);

	    ku_view_insert_subview(ui.basev, docview, ui.basev->views->length - 2);

	    vh_drag_drag(ui.dragv, docview);
	    REL(docview);

	    ui.dragdatav = event.selected_items;
	    ui.draggedv  = docview;
	}
	else if (event.id == KU_TABLE_EVENT_DROP)
	{
	    printf("FILE DROP\n");
	}
	else if (event.id == KU_TABLE_EVENT_CONTEXT)
	{
	    if (ui.contextcv->parent == NULL)
	    {
		if (event.rowview)
		{
		    ui.rowview_for_context_menu = event.rowview;

		    ku_view_t* contextpopup = ui.contextcv->views->data[0];
		    ku_rect_t  iframe       = contextpopup->frame.global;
		    iframe.x                = event.ev.x;
		    iframe.y                = event.ev.y;
		    ku_view_set_frame(contextpopup, iframe);

		    ku_view_add_subview(ui.basev, ui.contextcv);
		}
	    }
	}
    }
    else if (event.table == ui.cliptable)
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

	    ku_table_set_data(ui.infotable, items);

	    char* path = MGET(info, "file/path");
	    char* type = MGET(info, "file/type");

	    if (strcmp(type, "directory") != 0) ui_open(path);
	    ui_show_info(info);
	}
	else if (event.id == KU_TABLE_EVENT_DROP)
	{
	    if (ui.dragdatav)
	    {
		vec_add_in_vector(ui.clipdatav, ui.dragdatav);
		ku_table_set_data(ui.cliptable, ui.clipdatav);
	    }
	}
    }
    else if (event.table == ui.contexttable)
    {
	ku_view_remove_from_parent(ui.contextcv);
	if (event.id == KU_TABLE_EVENT_SELECT)
	{
	    if (event.selected_index == 0) // rename
	    {
		if (event.rowview)
		{
		    map_t* info = ui.filetable->selected_items->data[0];

		    ui.inputmode = UI_IM_RENAME;

		    ku_rect_t rframe = ui.rowview_for_context_menu->frame.global;
		    ku_rect_t iframe = ui.inputbckv->frame.global;
		    iframe.x         = rframe.x;
		    iframe.y         = rframe.y - 5;
		    ku_view_set_frame(ui.inputbckv, iframe);

		    ku_view_add_subview(ui.basev, ui.inputcv);

		    ku_view_layout(ui.basev);

		    ku_window_activate(ui.window, ui.inputtv);
		    vh_textinput_activate(ui.inputtv, 1);

		    char* value = MGET(info, "file/basename");
		    if (!value) value = "";

		    if (value)
		    {
			vh_textinput_set_text(ui.inputtv, value);
		    }
		}
	    }
	    else if (event.selected_index == 1) // delete
	    {
		ui_delete_selected();
	    }
	    else if (event.selected_index == 2) // send to cb
	    {
		vec_add_in_vector(ui.clipdatav, ui.filetable->selected_items);
		ku_table_set_data(ui.cliptable, ui.clipdatav);
	    }
	    else if (event.selected_index == 3) // paste using copy
	    {
		if (ui.current_folder)
		{
		    for (int index = 0; index < ui.clipdatav->length; index++)
		    {
			map_t* data    = ui.clipdatav->data[index];
			char*  path    = MGET(data, "file/path");
			char*  name    = MGET(data, "file/basename");
			char*  newpath = cstr_new_format(PATH_MAX, "%s/%s", ui.current_folder, name);

			int res = fm_copy(path, newpath);

			if (res < 0)
			{
			    ui_show_progress("Copy error");
			}
			else
			{
			    ui_show_progress("Files copied");
			    ui_load_folder(ui.current_folder);
			}
		    }
		}
	    }
	    else if (event.selected_index == 4) // paste using move
	    {
		for (int index = 0; index < ui.clipdatav->length; index++)
		{
		    map_t* data    = ui.clipdatav->data[index];
		    char*  path    = MGET(data, "file/path");
		    char*  name    = MGET(data, "file/basename");
		    char*  newpath = cstr_new_format(PATH_MAX, "%s/%s", ui.current_folder, name);

		    int res = fm_rename1(path, newpath);

		    if (res < 0)
		    {
			ui_show_progress("Move error");
		    }
		    else
		    {
			ui_show_progress("Files moved");
			ui_load_folder(ui.current_folder);
		    }
		}
		/* clear clipboard */
		vec_reset(ui.clipdatav);
		ku_table_set_data(ui.cliptable, ui.clipdatav);
	    }
	    else if (event.selected_index == 5) // reset clipboard
	    {
		vec_reset(ui.clipdatav);
		ku_table_set_data(ui.cliptable, ui.clipdatav);
	    }
	}
    }
}

void ui_toggle_pause()
{
    ui.play_state = 1 - ui.play_state;

    vh_button_set_state(ui.playbtnv, ui.play_state ? VH_BUTTON_UP : VH_BUTTON_DOWN);

    if (ui.media_state)
    {
	if (ui.play_state == 0 && !ui.media_state->paused)
	{
	    mp_pause(ui.media_state);
	}

	if (ui.play_state == 1 && ui.media_state->paused)
	{
	    mp_play(ui.media_state);
	}
    }
}

void on_cv_event(vh_cv_evnt_event_t event)
{
    if (event.id == VH_CV_EVENT_CLICK)
    {
	if (ui.media_type == UI_MT_DOCUMENT) ui_show_pdf_page(ui.pdf_page + 1);
	else if (ui.coder_type == CODER_MEDIA_TYPE_VIDEO || ui.coder_type == CODER_MEDIA_TYPE_AUDIO) ui_toggle_pause();
    }
}

void ui_cancel_input()
{
    ku_view_remove_subview(ui.basev, ui.inputcv);
}

void ui_on_touch(vh_touch_event_t event)
{
    if (strcmp(event.view->id, "inputcont") == 0)
    {
	ui_cancel_input();
    }
    else if (strcmp(event.view->id, "contextpopupcont") == 0)
    {
	ku_view_remove_from_parent(ui.contextcv);
    }
}

void ui_on_key_down(vh_key_event_t event)
{
    if (event.ev.keycode == XKB_KEY_space)
    {
	if (ui.media_type == UI_MT_DOCUMENT) ui_show_pdf_page(ui.pdf_page + 1);
	else if (ui.coder_type == CODER_MEDIA_TYPE_VIDEO || ui.coder_type == CODER_MEDIA_TYPE_AUDIO) ui_toggle_pause();
    }
    if (event.ev.keycode == XKB_KEY_Right)
    {
	if (ui.media_type == UI_MT_DOCUMENT) ui_show_pdf_page(ui.pdf_page + 1);
    }
    if (event.ev.keycode == XKB_KEY_Left)
    {
	if (ui.media_type == UI_MT_DOCUMENT) ui_show_pdf_page(ui.pdf_page - 1);
    }

    if (event.ev.keycode == XKB_KEY_Delete) ui_delete_selected();
    if (event.ev.keycode == XKB_KEY_c && event.ev.ctrl_down)
    {
	vec_add_in_vector(ui.clipdatav, ui.filetable->selected_items);
	ku_table_set_data(ui.cliptable, ui.clipdatav);
    }
    if (event.ev.keycode == XKB_KEY_v && event.ev.ctrl_down)
    {
	if (ui.rowview_for_context_menu)
	{
	    ku_rect_t  frame        = ui.rowview_for_context_menu->frame.global;
	    ku_view_t* contextpopup = ui.contextcv->views->data[0];
	    ku_rect_t  iframe       = contextpopup->frame.global;
	    iframe.x                = frame.x;
	    iframe.y                = frame.y;
	    ku_view_set_frame(contextpopup, iframe);
	    ku_view_add_subview(ui.basev, ui.contextcv);
	}
    }
    if (event.ev.keycode == XKB_KEY_plus || event.ev.keycode == XKB_KEY_KP_Add)
    {
	ku_view_t* evview = ku_view_get_subview(ui.basev, "previewevt");
	vh_cv_evnt_zoom(evview, 0.1);
    }
    if (event.ev.keycode == XKB_KEY_minus || event.ev.keycode == XKB_KEY_KP_Subtract)
    {
	ku_view_t* evview = ku_view_get_subview(ui.basev, "previewevt");
	vh_cv_evnt_zoom(evview, -0.1);
    }
}

void ui_on_btn_event(vh_button_event_t event)
{
    ku_view_t* btnview = event.view;

    printf("%s\n", btnview->id);

    if (strcmp(event.view->id, "prevbtn") == 0) ui_show_pdf_page(ui.pdf_page - 1);
    else if (strcmp(event.view->id, "nextbtn") == 0) ui_show_pdf_page(ui.pdf_page + 1);
    else if (strcmp(event.view->id, "playbtn") == 0)
    {
	if (event.vh->state == VH_BUTTON_DOWN)
	{
	    ui.play_state = 0;
	    if (ui.media_state) mp_pause(ui.media_state);
	}
	else
	{
	    ui.play_state = 1;
	    if (ui.media_state) mp_play(ui.media_state);
	}
    }
    else if (strcmp(event.view->id, "settingsclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.settingscv);
    }
    else if (strcmp(event.view->id, "pathprevbtn") == 0)
    {
    }
    else if (strcmp(event.view->id, "pathclearbtn") == 0)
    {
	vh_textinput_set_text(ui.pathtv, "");
	ku_window_activate(ui.window, ui.pathtv);
	vh_textinput_activate(ui.pathtv, 1);
    }
    else if (strcmp(event.view->id, "plusbtn") == 0)
    {
	ku_view_t* evview = ku_view_get_subview(ui.basev, "previewevnt");
	vh_cv_evnt_zoom(evview, 0.1);
    }
    else if (strcmp(event.view->id, "minusbtn") == 0)
    {
	ku_view_t* evview = ku_view_get_subview(ui.basev, "previewevnt");
	vh_cv_evnt_zoom(evview, -0.1);
    }
    else if (strcmp(event.view->id, "okayacceptbtn") == 0)
    {
	if (ui.filetable->selected_items->length > 0)
	{
	    for (int index = 0; index < ui.filetable->selected_items->length; index++)
	    {
		ku_view_remove_from_parent(ui.okaycv);
		map_t* file = ui.filetable->selected_items->data[index];
		char*  path = MGET(file, "file/path");
		fm_delete(path);
		ui_load_folder(ui.current_folder);
	    }
	}
    }
    else if (strcmp(event.view->id, "okayclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.okaycv);
    }
    else if (strcmp(event.view->id, "closebtn") == 0)
    {
	ku_wayland_exit();
    }
    else if (strcmp(event.view->id, "maxbtn") == 0) ku_wayland_toggle_fullscreen(ui.window);
    else if (strcmp(event.view->id, "sidebarbtn") == 0)
    {
	ku_view_t* top = ku_view_get_subview(ui.basev, "top_flex");

	if (ui.infotablev->parent)
	{
	    ku_view_remove_from_parent(ui.infotablev);
	    ku_view_add_subview(top, ui.cliptablev);

	    ku_window_set_focusable(ui.window, ui.focusable_cliplist);

	    config_set_bool("sidebar_visible", 1);
	}
	else if (ui.cliptablev->parent)
	{
	    ku_view_remove_from_parent(ui.cliptablev);
	    ku_window_set_focusable(ui.window, ui.focusable_filelist);

	    config_set_bool("sidebar_visible", 0);
	}
	else
	{
	    ku_view_add_subview(top, ui.infotablev);
	    ku_window_set_focusable(ui.window, ui.focusable_infolist);

	    config_set_bool("sidebar_visible", 1);
	}

	config_write(config_get("cfg_path"));
	ku_view_layout(top);
    }
    else if (strcmp(event.view->id, "settingsbtn") == 0)
    {
	if (!ui.settingscv->parent)
	{
	    ku_view_add_subview(ui.basev, ui.settingscv);
	    ku_view_layout(ui.basev);
	}
    }
}

void ui_on_slider_event(vh_slider_event_t event)
{
    if (ui.media_state)
    {
	if (ui.media_state->paused) mp_play(ui.media_state);
	mp_set_position(ui.media_state, event.ratio);
    }

    if (ui.media_type == UI_MT_DOCUMENT)
    {
	int new_page = (int) (roundf(((float) (ui.pdf_page_count - 1) * event.ratio)));

	if (new_page != ui.pdf_page) ui_show_pdf_page(new_page);
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

	    if (ui.filetable->selected_items->length > 0)
	    {
		map_t* file   = ui.filetable->selected_items->data[0];
		char*  parent = MGET(file, "file/parent");

		char* oldpath = MGET(file, "file/path");
		char* newpath = path_new_append(parent, event.text);

		fm_rename(oldpath, newpath, NULL);

		REL(newpath);

		ui_load_folder(ui.current_folder);
	    }
	}
    }
    else if (strcmp(event.view->id, "pathtf") == 0)
    {
	if (event.id == VH_TEXTINPUT_DEACTIVATE) vh_textinput_activate(ui.pathtv, 0);

	if (event.id == VH_TEXTINPUT_RETURN)
	{
	    vh_textinput_activate(ui.pathtv, 0);
	    ku_window_deactivate(ui.window, ui.pathtv);

	    char* valid_path = cstr_new_cstring(event.text);
	    for (;;)
	    {
		if (fm_exists(valid_path)) break;
		else
		{
		    char* prev_path = path_new_remove_last_component(valid_path);
		    REL(valid_path);
		    valid_path = prev_path;
		}
	    }
	    ui_load_folder(valid_path);
	    REL(valid_path);
	}
    }
}

void ui_on_drag(vh_drag_event_t event)
{
    if (event.id == VH_DRAG_DROP)
    {
	if (ui.draggedv) ku_view_remove_from_parent(ui.draggedv);
    }
}

void ui_add_cursor()
{
    ui.cursorv                         = ku_view_new("ui.cursor", ((ku_rect_t){10, 10, 10, 10}));
    ui.cursorv->style.background_color = 0xFF000099;
    ui.cursorv->needs_touch            = 0;
    tg_css_add(ui.cursorv);
    ku_window_add(ui.window, ui.cursorv);
}

void ui_update_cursor(ku_rect_t frame)
{
    ku_view_set_frame(ui.cursorv, frame);
}

int ui_comp_text(void* left, void* right)
{
    char* la = left;
    char* ra = right;

    return strcmp(la, ra);
}

ku_table_t* ui_create_table(ku_view_t* view, vec_t* fields)
{
    /* <div id="filetable" class="colflex marginlt4"> */
    /*   <div id="filetablehead" class="tablehead overflowhidden"/> */
    /*   <div id="filetablelayers" class="black fullscaleview overflowhidden"> */
    /*     <div id="filetablebody" class="tablebody fullscaleview"> */
    /* 	     <div id="filetable_row_a" class="rowa" type="label" text="row a"/> */
    /* 	     <div id="filetable_row_b" class="rowb" type="label" text="row b"/> */
    /* 	     <div id="filetable_row_selected" class="rowselected" type="label" text="row selected"/> */
    /*     </div> */
    /*     <div id="filetablescroll" class="fullscaleview"> */
    /* 	     <div id="filetablevertscroll" class="vertscroll"/> */
    /* 	     <div id="filetablehoriscroll" class="horiscroll"/> */
    /*     </div> */
    /*     <div id="filetableevt" class="fullscaleview"/> */
    /*   </div> */
    /* </div> */

    ku_view_t* header  = NULL;
    ku_view_t* headrow = NULL;
    ku_view_t* layers  = NULL;
    ku_view_t* body    = NULL;
    ku_view_t* scroll  = NULL;
    ku_view_t* event   = NULL;
    ku_view_t* row_a   = NULL;
    ku_view_t* row_b   = NULL;
    ku_view_t* row_s   = NULL;

    if (view->views->length == 2)
    {
	header  = view->views->data[0];
	headrow = header->views->data[0];
	layers  = view->views->data[1];
	body    = layers->views->data[0];
	scroll  = layers->views->data[1];
	event   = layers->views->data[2];
	row_a   = body->views->data[0];
	row_b   = body->views->data[1];
	row_s   = body->views->data[2];
    }
    if (view->views->length == 1)
    {
	layers = view->views->data[0];
	body   = layers->views->data[0];
	event  = layers->views->data[1];
	row_a  = body->views->data[0];
	row_b  = body->views->data[1];
	row_s  = body->views->data[2];
    }

    textstyle_t rowastyle = ku_util_gen_textstyle(row_a);
    textstyle_t rowbstyle = ku_util_gen_textstyle(row_b);
    textstyle_t rowsstyle = ku_util_gen_textstyle(row_s);
    textstyle_t headstyle = headrow == NULL ? (textstyle_t){0} : ku_util_gen_textstyle(headrow);

    ku_table_t* table = ku_table_create(
	view->id,
	body,
	scroll,
	event,
	header,
	fields,
	rowastyle,
	rowbstyle,
	rowsstyle,
	headstyle,
	on_table_event);

    ku_view_remove_from_parent(row_a);
    ku_view_remove_from_parent(row_b);
    ku_view_remove_from_parent(row_s);
    if (headrow) ku_view_remove_from_parent(headrow);

    return table;
}

void ui_init(int width, int height, float scale, ku_window_t* window)
{
    ku_text_init(); // DESTROY 0

    ui.window         = window;
    ui.play_state     = 1;
    ui.folder_history = MNEW();

    ui.filedatav = VNEW(); // REL S0
    ui.clipdatav = VNEW(); // REL S0

    /* generate views from descriptors */

    vec_t* view_list = VNEW();

    ku_gen_html_parse(config_get("html_path"), view_list);
    ku_gen_css_apply(view_list, config_get("css_path"), config_get("res_path"), 1.0);
    ku_gen_type_apply(view_list, ui_on_btn_event, ui_on_slider_event);

    ui.basev = RET(vec_head(view_list));

    REL(view_list);

    /* initial layout of views */

    ku_view_set_frame(ui.basev, (ku_rect_t){0.0, 0.0, (float) width, (float) height});
    ku_window_add(ui.window, ui.basev);

    /* listen for keys and shortcuts */

    vh_key_add(ui.basev, ui_on_key_down);
    ui.basev->needs_key = 1;
    ku_window_activate(ui.window, ui.basev);

    /* setup drag layer */

    ui.dragv = ku_view_get_subview(ui.basev, "draglayer");
    vh_drag_attach(ui.dragv, ui_on_drag);

    /* set up focusable */

    ku_view_t* filetableevnt = ku_view_get_subview(ui.basev, "filetableevt");
    ku_view_t* infotableevt  = ku_view_get_subview(ui.basev, "infotableevt");
    ku_view_t* cliptableevt  = ku_view_get_subview(ui.basev, "cliptableevt");
    ku_view_t* previewevt    = ku_view_get_subview(ui.basev, "previewevt");
    ku_view_t* pathtv        = ku_view_get_subview(ui.basev, "pathtf");

    ui.focusable_filelist = VNEW();
    ui.focusable_infolist = VNEW();
    ui.focusable_cliplist = VNEW();

    VADDR(ui.focusable_filelist, filetableevnt);
    VADDR(ui.focusable_filelist, previewevt);
    VADDR(ui.focusable_filelist, pathtv);

    VADDR(ui.focusable_infolist, filetableevnt);
    VADDR(ui.focusable_infolist, infotableevt);
    VADDR(ui.focusable_infolist, previewevt);
    VADDR(ui.focusable_infolist, pathtv);

    VADDR(ui.focusable_cliplist, filetableevnt);
    VADDR(ui.focusable_cliplist, cliptableevt);
    VADDR(ui.focusable_cliplist, previewevt);
    VADDR(ui.focusable_cliplist, pathtv);

    /* setup visualizer */

    ui.prevcontv = ku_view_get_subview(ui.basev, "previewcont");
    ui.prevbodyv = ku_view_get_subview(ui.basev, "previewbody");

    /* files table */

    vec_t* fields = VNEW();
    VADDR(fields, cstr_new_cstring("file/basename"));
    VADDR(fields, num_new_int(400));
    VADDR(fields, cstr_new_cstring("file/size"));
    VADDR(fields, num_new_int(120));
    VADDR(fields, cstr_new_cstring("file/last_access"));
    VADDR(fields, num_new_int(180));
    VADDR(fields, cstr_new_cstring("file/last_modification"));
    VADDR(fields, num_new_int(180));
    VADDR(fields, cstr_new_cstring("file/last_status"));
    VADDR(fields, num_new_int(180));

    ku_view_t* filetable    = ku_view_get_subview(ui.basev, "filetable");
    ku_view_t* filetableevt = ku_view_get_subview(ui.basev, "filetableevt");

    ui.filetable = ui_create_table(filetable, fields);
    ku_window_activate(ui.window, filetableevt); // start with file list listening for key up and down

    /* clipboard table */

    ku_view_t* cliptable = ku_view_get_subview(ui.basev, "cliptable");
    ui.cliptable         = ui_create_table(cliptable, fields);

    REL(fields);

    /* file info table */

    fields = VNEW();

    VADDR(fields, cstr_new_cstring("key"));
    VADDR(fields, num_new_int(150));
    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(400));

    ku_view_t* infotable = ku_view_get_subview(ui.basev, "infotable");

    ui.infotable = ui_create_table(infotable, fields);

    REL(fields);

    /* context popup table */

    fields = VNEW();

    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(200));

    ku_view_t* contexttable = ku_view_get_subview(ui.basev, "contexttable");

    ui.contexttable = ui_create_table(contexttable, fields);

    REL(fields);

    vec_t* items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Rename")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Delete")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Send to clipboard")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Paste using copy")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Paste using move")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(50, "Reset clipboard")}));

    ku_table_set_data(ui.contexttable, items);
    REL(items);

    /* settings table */

    fields = VNEW();

    VADDR(fields, cstr_new_cstring("value"));
    VADDR(fields, num_new_int(510));

    ku_view_t* settingstable = ku_view_get_subview(ui.basev, "settingstable");
    ui.settingstable         = ui_create_table(settingstable, fields);

    REL(fields);

    items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "MultiMedia File Manager v%s beta by Milan Toth", MMFM_VERSION)}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Free and Open Source Software")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", cstr_new_format(200, "Donate on Paypal")}));

    ku_table_set_data(ui.settingstable, items);
    REL(items);

    /* preview block */

    ku_view_t* preview     = ku_view_get_subview(ui.basev, "preview");
    ku_view_t* previewbody = ku_view_get_subview(ui.basev, "previewbody");
    ku_view_t* previewscrl = ku_view_get_subview(ui.basev, "previewscrl");
    ku_view_t* previewevnt = ku_view_get_subview(ui.basev, "previewevt");
    ku_view_t* previewcont = ku_view_get_subview(ui.basev, "previewcont");

    if (preview)
    {
	tg_scaledimg_add(previewcont, 30, 30);
	ku_view_set_frame(previewcont, (ku_rect_t){0, 0, 30, 30});
	previewcont->style.margin = INT_MAX;

	vh_cv_body_attach(previewbody, NULL);
	vh_cv_scrl_attach(previewscrl, previewbody, NULL);
	vh_cv_evnt_attach(previewevnt, previewbody, previewscrl, NULL, on_cv_event);
    }
    else
	zc_log_debug("preview not found");

    ku_view_t* settingsclosebtn = ku_view_get_subview(ui.basev, "settingsclosebtn");
    vh_button_add(settingsclosebtn, VH_BUTTON_NORMAL, ui_on_btn_event);

    // get main bottom for layout change

    ui.mainbottomv = ku_view_get_subview(ui.basev, "main_bottom");

    /* settings popup */

    ku_view_t* settingscv    = ku_view_get_subview(ui.basev, "settingspopupcont");
    ku_view_t* settingspopup = ku_view_get_subview(ui.basev, "settingspopup");

    settingspopup->blocks_touch  = 1;
    settingspopup->blocks_scroll = 1;

    ui.settingscv = RET(settingscv);
    ku_view_remove_from_parent(ui.settingscv);

    /* context popup */

    ku_view_t* contextcv    = ku_view_get_subview(ui.basev, "contextpopupcont");
    ku_view_t* contextpopup = ku_view_get_subview(ui.basev, "contextpopup");

    contextpopup->blocks_touch  = 1;
    contextpopup->blocks_scroll = 1;

    ui.contextcv = RET(contextcv);

    vh_touch_add(ui.contextcv, ui_on_touch);
    ku_view_remove_from_parent(ui.contextcv);

    /* okay popup */

    ku_view_t* okaycv    = ku_view_get_subview(ui.basev, "okaypopupcont");
    ku_view_t* okaypopup = ku_view_get_subview(ui.basev, "okaypopup");

    okaypopup->blocks_touch  = 1;
    okaypopup->blocks_scroll = 1;

    ui.okaycv = RET(okaycv);

    vh_touch_add(ui.okaycv, ui_on_touch);
    ku_view_remove_from_parent(ui.okaycv);

    // info textfield

    ui.statuslv = ku_view_get_subview(ui.basev, "statustf");
    ui_show_progress("Directory loaded");

    /* input popup */

    ui.inputcv   = RET(ku_view_get_subview(ui.basev, "inputcont"));
    ui.inputbckv = ku_view_get_subview(ui.basev, "inputbck");
    ui.inputtv   = ku_view_get_subview(ui.basev, "inputtf");

    ui.inputbckv->blocks_touch = 1;
    ui.inputcv->blocks_touch   = 1;
    ui.inputcv->blocks_scroll  = 1;

    vh_textinput_add(ui.inputtv, "Generic input", "", ui_on_text_event);

    vh_touch_add(ui.inputcv, ui_on_touch);

    ku_view_remove_from_parent(ui.inputcv);

    /* path bar */

    ui.pathtv = ku_view_get_subview(ui.basev, "pathtf");

    vh_textinput_add(ui.pathtv, "/home/milgra", "", ui_on_text_event);

    /* time label */

    ui.timelv = ku_view_get_subview(ui.basev, "timetf");

    ui.seekbarv = ku_view_get_subview(ui.basev, "seekbar");

    // main gap

    ui.cliptablev = RET(ku_view_get_subview(ui.basev, "cliptable"));
    ui.infotablev = RET(ku_view_get_subview(ui.basev, "infotable"));

    if (config_get_bool("sidebar_visible") == 0)
    {
	ku_view_remove_from_parent(ui.cliptablev);
	ku_view_remove_from_parent(ui.infotablev);

	ku_window_set_focusable(window, ui.focusable_filelist);
    }
    else
    {
	ku_view_remove_from_parent(ui.cliptablev);
	ku_window_set_focusable(window, ui.focusable_infolist);
    }

    ui.playbtnv  = RET(ku_view_get_subview(ui.basev, "playbtn"));
    ui.prevbtnv  = RET(ku_view_get_subview(ui.basev, "prevbtn"));
    ui.nextbtnv  = RET(ku_view_get_subview(ui.basev, "nextbtn"));
    ui.plusbtnv  = RET(ku_view_get_subview(ui.basev, "plusbtn"));
    ui.minusbtnv = RET(ku_view_get_subview(ui.basev, "minusbtn"));

    vh_button_disable(ui.playbtnv);
    vh_button_disable(ui.nextbtnv);
    vh_button_disable(ui.prevbtnv);
    vh_button_disable(ui.plusbtnv);
    vh_button_disable(ui.minusbtnv);
    vh_slider_disable(ui.seekbarv);

    // show texture map for debug

    /* ku_view_t* texmap       = ku_view_new("texmap", ((ku_rect_t){0, 0, 150, 150})); */
    /* texmap->needs_touch  = 0; */
    /* texmap->texture.full = 1; */
    /* texmap->style.right  = 0; */
    /* texmap->style.top    = 0; */

    /* ku_window_add(ui.window,texmap); */
}

void ui_destroy()
{
    if (ui.media_state)
    {
	mp_close(ui.media_state);
	ui.media_state = NULL;
    }

    ku_window_remove(ui.window, ui.basev);

    REL(ui.cliptablev);
    REL(ui.infotablev);

    REL(ui.basev);
    REL(ui.infotable);
    REL(ui.filetable);
    REL(ui.cliptable);
    REL(ui.settingstable);
    REL(ui.contexttable);

    REL(ui.filedatav);
    REL(ui.clipdatav);

    REL(ui.settingscv);
    REL(ui.contextcv);

    REL(ui.inputcv);

    if (ui.pdf_path) REL(ui.pdf_path);

    ku_text_destroy(); // DESTROY 0
}

void ui_update_layout(int w, int h)
{
    if (w > h) ui.mainbottomv->style.flexdir = FD_ROW;
    else ui.mainbottomv->style.flexdir = FD_COL;

    ku_view_layout(ui.basev);
}

void ui_update_player()
{
    if (ui.media_state)
    {
	double rem = 0.01;
	mp_video_refresh(ui.media_state, &rem, ui.prevcontv->texture.bitmap);
	// mp_audio_refresh(ui.media_state, ui.prevcontv->texture.bitmap, ui.prevcontv->texture.bitmap);

	double time = roundf(mp_get_master_clock(ui.media_state) * 10.0) / 10.0;

	if (ui.play_state == 0 && !ui.media_state->paused)
	{
	    if (time > 0.1) mp_pause(ui.media_state);
	}

	if (time != ui.last_update && !isnan(time))
	{
	    ui.last_update = time;

	    int tmin = (int) floor(time / 60.0);
	    int tsec = (int) time % 60;
	    int dmin = (int) floor(ui.media_state->duration / 60.0);
	    int dsec = (int) ui.media_state->duration % 60;

	    char timebuff[32];
	    snprintf(timebuff, 32, "%.2i:%.2i / %.2i:%.2i", tmin, tsec, dmin, dsec);
	    tg_text_set1(ui.timelv, timebuff);

	    double ratio = time / ui.media_state->duration;

	    vh_slider_set(ui.seekbarv, ratio);
	}

	ui.prevcontv->texture.changed = 1;
    }
}

#endif

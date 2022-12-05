#ifndef ui_h
#define ui_h

#include "ku_view.c"
#include "ku_window.c"
#include "mt_map.c"

typedef enum _ui_inputmode ui_inputmode;
enum _ui_inputmode
{
    UI_IM_RENAME,
    UI_IM_NEWFOLDER,
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
void ui_load_file(char* path);
void ui_open_file(mt_map_t* info);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "coder.c"
#include "config.c"
#include "filemanager.c"
#include "ku_connector_wayland.c"
#include "ku_fontconfig.c"
#include "ku_gen_css.c"
#include "ku_gen_html.c"
#include "ku_gen_textstyle.c"
#include "ku_gen_type.c"
#include "ku_gl.c"
#include "mediaplayer.c"
#include "mt_log.c"
#include "mt_map_ext.c"
#include "mt_number.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_time.c"
#include "pdf.c"
#include "tg_css.c"
#include "tg_scaledimg.c"
#include "tg_text.c"
#include "vh_anim.c"
#include "vh_button.c"
#include "vh_cv_body.c"
#include "vh_cv_evnt.c"
#include "vh_cv_scrl.c"
#include "vh_drag.c"
#include "vh_key.c"
#include "vh_table.c"
#include "vh_tbl_body.c"
#include "vh_tbl_scrl.c"
#include "vh_textinput.c"
#include "vh_touch.c"
#include <limits.h>

struct _ui_t
{
    ku_window_t* window; /* window for this ui */

    ku_view_t* basev;       /* base view */
    ku_view_t* dragv;       /* drag overlay */
    ku_view_t* cursorv;     /* replay cursor */
    ku_view_t* draggedv;    /* file drag icon */
    ku_view_t* prevbodyv;   /* preview boody */
    ku_view_t* prevcontv;   /* preview container */
    ku_view_t* inputbckv;   /* input textfield background for positioning */
    ku_view_t* mainbottomv; /* main bottom container */

    ku_view_t* seeklv;   /* seek label */
    ku_view_t* statuslv; /* status label */
    ku_view_t* pathtv;   /* path text view */
    ku_view_t* inputtv;  /* input text view */

    ku_view_t* settingscv; /* settings popup container view */
    ku_view_t* contextcv;  /* context popup container view */
    ku_view_t* inputcv;    /* input popup container view */
    ku_view_t* okaycv;     /* okay popup container view */
    ku_view_t* okaypopup;  /* okay popup view for grabbing key events */

    ku_view_t* seekbarv;
    ku_view_t* minusbtnv;
    ku_view_t* plusbtnv;
    ku_view_t* playbtnv;
    ku_view_t* prevbtnv;
    ku_view_t* nextbtnv;

    ku_view_t* cliptablev;
    ku_view_t* filetablev;
    ku_view_t* infotablev;
    ku_view_t* contexttablev;
    ku_view_t* settingstablev;

    mt_vector_t* focusable_filelist; /* focusable views in file list only mode */
    mt_vector_t* focusable_infolist; /* focusable views in file + info list mode */
    mt_vector_t* focusable_cliplist; /* focusable views in file + clipboard list mode */

    /* text editing */

    ui_inputmode inputmode;

    /* data containers */

    mt_vector_t* filedatav; /* files table data */
    mt_vector_t* clipdatav; /* clipboard table data */
    mt_vector_t* dragdatav; /* dragged files data */

    /* file browser state */

    char*      current_folder;
    mt_map_t*  folder_history;
    ku_view_t* rowview_for_context_menu; /* TODO do we need this? */

    /* media state */

    MediaState_t*      media_state;
    ui_media_type      media_type;  /* media type of ui */
    coder_media_type_t coder_type;  /* media type of stream TODO these two should be merged */
    int                play_state;  /* play or pause selected last */
    float              last_update; /* last update of time label */

    int   pdf_page_count;
    int   pdf_page;
    char* pdf_path;
} ui;

/* adds cursor for replay/record */

void ui_add_cursor()
{
    ui.cursorv                         = ku_view_new("ui.cursor", ((ku_rect_t){10, 10, 10, 10}));
    ui.cursorv->style.background_color = 0xFF000099;
    ui.cursorv->needs_touch            = 0;
    tg_css_add(ui.cursorv);
    ku_window_add(ui.window, ui.cursorv);
}

/* updates cursor position */

void ui_update_cursor(ku_rect_t frame)
{
    ku_view_set_frame(ui.cursorv, frame);
}

/* switch flex direction on resize  */

void ui_update_layout(int w, int h)
{
    /* if (w > h) ui.mainbottomv->style.flexdir = FD_ROW; */
    /* else ui.mainbottomv->style.flexdir = FD_COL; */

    /* ku_view_layout(ui.basev, ui.basev->style.scale); */
}

void ui_rotate_sidebar()
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
    ku_view_layout(top, ui.basev->style.scale);
}

/* update player, seek bar position and video textures */

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
	    tg_text_set1(ui.seeklv, timebuff);

	    double ratio = time / ui.media_state->duration;

	    vh_slider_set(ui.seekbarv, ratio);
	}

	ui.prevcontv->texture.changed = 1;
    }
}

/* shows status message in status bar */

void ui_show_status(const char* format, ...)
{
    char    buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 256, format, args);
    va_end(args);

    tg_text_set1(ui.statuslv, buffer);
}

/* opens/shows pdf page in preview content view */

void ui_show_pdf_page(int page)
{
    if (page == ui.pdf_page_count) page--;
    if (page == -1) page++;

    if (page != ui.pdf_page)
    {
	ui.pdf_page = page;

	/* update seek text */
	char buff[32];
	snprintf(
	    buff,
	    32,
	    "Page %i / %i",
	    ui.pdf_page + 1,
	    ui.pdf_page_count);

	tg_text_set1(ui.seeklv, buff);

	/* update seek slider */
	vh_slider_set(ui.seekbarv, (float) (ui.pdf_page + 1) / (float) ui.pdf_page_count);

	/* generate bitmap */
	/* TODO : render directly into texture after getting dimensions to speed up things */
	ku_bitmap_t* pdfbmp = pdf_render(ui.pdf_path, ui.pdf_page);

	vh_cv_body_set_content_size(ui.prevbodyv, pdfbmp->w, pdfbmp->h);

	/* add to preview container */
	if (ui.prevcontv->texture.bitmap != NULL)
	{
	    ku_draw_insert(ui.prevcontv->texture.bitmap, pdfbmp, 0, 0);
	    ku_view_upload_texture(ui.prevcontv);
	}

	REL(pdfbmp);
    }
}

/* pause or play streaming media and save state */

void ui_toggle_pause()
{
    ui.play_state = 1 - ui.play_state;

    vh_button_set_state(ui.playbtnv, ui.play_state ? VH_BUTTON_UP : VH_BUTTON_DOWN);

    if (ui.media_state)
    {
	if (ui.play_state == 0 && !ui.media_state->paused) mp_pause(ui.media_state);
	if (ui.play_state == 1 && ui.media_state->paused) mp_play(ui.media_state);
    }
}

/* media player event, content resize on stream start/stream switching currently */

void ui_on_mp_event(ms_event_t event)
{
    vh_cv_body_set_content_size(ui.prevbodyv, (int) event.rect.x, (int) event.rect.y);
}

/* entry comparator for folder loading */

int ui_comp_entry(void* left, void* right)
{
    mt_map_t* l = left;
    mt_map_t* r = right;

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

/* opens folder in files table */

void ui_load_folder(char* folder)
{
    if (folder)
    {
	vh_table_t* vh            = ui.filetablev->handler_data;
	int         prev_selected = vh->selected_index;

	/* prev selected should stay after delete */
	if (folder != ui.current_folder)
	{
	    prev_selected = 0;

	    /* store folder as last visited */
	    if (ui.current_folder) MPUT(ui.folder_history, ui.current_folder, folder);
	    if (ui.current_folder) REL(ui.current_folder);
	    ui.current_folder = mt_path_new_normalize1(folder);
	}

	/* show in path bar */
	vh_textinput_set_text(ui.pathtv, ui.current_folder);

	/* read folder */
	mt_map_t* files = MNEW();
	fm_list(ui.current_folder, files);

	mt_vector_reset(ui.filedatav);
	mt_map_values(files, ui.filedatav);
	mt_vector_sort(ui.filedatav, ui_comp_entry);

	vh_table_set_data(ui.filetablev, ui.filedatav);
	vh_table_select(ui.filetablev, prev_selected, 0);
	REL(files);

	/* jump to item if there is a last visited item */
	char* last_visited = MGET(ui.folder_history, ui.current_folder);
	if (last_visited)
	{
	    int index = 0;
	    int found = 0;
	    for (index = 0; index < ui.filedatav->length; index++)
	    {
		mt_map_t* info = ui.filedatav->data[index];
		char*     path = MGET(info, "file/path");

		printf("LAST VIS %s PATH %s\n", last_visited, path);
		if (strcmp(path, last_visited) == 0)
		{
		    found = 1;
		    break;
		}
	    }
	    if (found) vh_table_select(ui.filetablev, index, 0);
	}

	ui_show_status("Directory %s loaded", ui.current_folder);
    }
}

void ui_load_file(char* path)
{
    if (path)
    {
	mt_map_t* file = MNEW();

	MPUTR(file, "file/type", mt_string_new_cstring("file"));
	MPUTR(file, "file/path", mt_string_new_cstring(path));

	ui_open_file(file);

	REL(file);
    }
}

void ui_open_folder(mt_map_t* info)
{
    char* type = MGET(info, "file/type");
    char* path = MGET(info, "file/path");

    if (strcmp(type, "directory") == 0) ui_load_folder(path);
}

/* enable/disable control buttons based on media type */

void ui_update_control_bar()
{
    int play_flag = ui.coder_type == CODER_MEDIA_TYPE_VIDEO || ui.coder_type == CODER_MEDIA_TYPE_AUDIO;
    int prev_flag = ui.media_type == UI_MT_DOCUMENT;
    int next_flag = ui.media_type == UI_MT_DOCUMENT;
    int seek_flag = ui.media_type == UI_MT_DOCUMENT || ui.coder_type == CODER_MEDIA_TYPE_VIDEO || ui.coder_type == CODER_MEDIA_TYPE_AUDIO;
    int zoom_flag = ui.coder_type != CODER_MEDIA_TYPE_OTHER || ui.media_type == UI_MT_DOCUMENT;

    vh_button_set_enabled(ui.playbtnv, play_flag);
    vh_button_set_enabled(ui.prevbtnv, prev_flag);
    vh_button_set_enabled(ui.nextbtnv, next_flag);
    vh_button_set_enabled(ui.plusbtnv, zoom_flag);
    vh_button_set_enabled(ui.minusbtnv, zoom_flag);
    vh_slider_set_enabled(ui.seekbarv, seek_flag);

    if (ui.media_type != UI_MT_DOCUMENT) tg_text_set1(ui.seeklv, "");
}

/* open media/document file */

void ui_open_file(mt_map_t* info)
{
    char* path = MGET(info, "file/path");
    char* type = MGET(info, "file/type");

    if (strcmp(type, "directory") != 0)
    {
	if (ui.media_state)
	{
	    mp_close(ui.media_state);
	    ui.media_state = NULL;
	}

	if (strstr(path, ".pdf") != NULL)
	{
	    if (ui.pdf_path) REL(ui.pdf_path);
	    ui.pdf_path       = RET(path);
	    ui.pdf_page       = -1;
	    ui.pdf_page_count = pdf_count(path);
	    ui.media_type     = UI_MT_DOCUMENT;
	    ui_show_pdf_page(0);
	}
	else
	{
	    ui.coder_type = coder_get_type(path);

	    if (ui.coder_type == CODER_MEDIA_TYPE_VIDEO ||
		ui.coder_type == CODER_MEDIA_TYPE_AUDIO)
	    {
		/* open with media player */
		ui.media_state = mp_open(path, ui_on_mp_event);
	    }
	    else if (ui.coder_type == CODER_MEDIA_TYPE_IMAGE)
	    {
		/* decode image immediately and load into preview content view */
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
	    }

	    ui.media_type = UI_MT_STREAM;
	}

	ui_update_control_bar();
    }
}

/* show file information in info table */

void ui_show_info(mt_map_t* info)
{
    if (!MGET(info, "file/mime")) fm_detail(info); /* request more info if file not yet detailed */

    mt_vector_t* keys = VNEW();
    mt_map_keys(info, keys);
    mt_vector_sort(keys, ((int (*)(void*, void*)) strcmp));

    mt_vector_t* items = VNEW();
    for (int index = 0; index < keys->length; index++)
    {
	char*     key = keys->data[index];
	mt_map_t* map = MNEW();
	MPUT(map, "key", key);
	MPUT(map, "value", MGET(info, key));
	VADDR(items, map);
    }

    REL(keys);

    vh_table_set_data(ui.infotablev, items);
    ui_show_status("File info/data loaded for %s", MGET(info, "file/basename"));

    REL(items);
}

/* delete selected files */

void ui_open_approve_popup()
{
    vh_table_t* vh = ui.filetablev->handler_data;
    if (vh->selected_items->length > 0)
    {
	if (ui.okaycv->parent == NULL)
	{
	    ku_view_add_subview(ui.basev, ui.okaycv);
	    ku_view_layout(ui.basev, ui.basev->style.scale);
	    ku_window_activate(ui.window, ui.okaypopup, 1);
	}
    }
}

void ui_delete_selected_files()
{
    vh_table_t* vh = ui.filetablev->handler_data;
    if (vh->selected_items->length > 0)
    {
	for (int index = 0; index < vh->selected_items->length; index++)
	{
	    mt_map_t* file = vh->selected_items->data[index];
	    char*     path = MGET(file, "file/path");
	    fm_delete(path);
	}
	ui_load_folder(ui.current_folder);
    }
}

/* show drag icon */

void ui_show_drag_view()
{
    ku_view_t* docview              = ku_view_new("dragged_view", ((ku_rect_t){.x = 0, .y = 0, .w = 50, .h = 50}));
    docview->style.background_color = 0x00FF00FF;
    tg_css_add(docview);

    ui.draggedv = docview;

    ku_view_insert_subview(ui.basev, docview, ui.basev->views->length - 2);

    vh_drag_drag(ui.dragv, docview);
    REL(docview);
}

/* show context menu */

void ui_show_context_menu(float x, float y)
{
    if (ui.contextcv->parent == NULL)
    {
	ku_view_t* contextpopup = ui.contextcv->views->data[0];
	ku_rect_t  iframe       = contextpopup->frame.global;
	iframe.x                = x + 20.0;
	iframe.y                = y;
	ku_view_add_subview(ui.basev, ui.contextcv);
	ku_view_set_frame(contextpopup, iframe);

	ku_view_t* contexttableevt = GETV(ui.contextcv, "contexttable_event");
	ku_window_activate(ui.window, contexttableevt, 1);

	ku_rect_t start = contextpopup->frame.local;

	ku_rect_t end = start;

	start.x += 40;
	start.w -= 80;
	start.h = 10;
	ku_view_set_frame(contextpopup, start);

	vh_anim_frame(contextpopup, start, end, 0, 15, AT_EASE);

	ku_view_t* contextanim = GETV(contextpopup, "contextpopupanim");

	start = contextanim->frame.local;
	end   = start;

	start.w -= 80;
	start.h = 10;
	ku_view_set_frame(contextanim, start);

	vh_anim_frame(contextanim, start, end, 0, 15, AT_EASE);
    }
}

/* show input field over labels */

void ui_show_input_popup(float x, float y, char* text)
{
    ku_rect_t iframe = ui.inputbckv->frame.global;
    iframe.x         = x;
    iframe.y         = y;
    ku_view_set_frame(ui.inputbckv, iframe);
    ku_view_add_subview(ui.basev, ui.inputcv);
    ku_view_layout(ui.basev, ui.basev->style.scale);
    ku_window_activate(ui.window, ui.inputtv, 1);
    vh_textinput_activate(ui.inputtv, 1);
    vh_textinput_set_text(ui.inputtv, text);
    vh_textinput_set_limit(ui.inputtv, 255);
}

/* cancel input */

void ui_cancel_input()
{
    ku_view_remove_subview(ui.basev, ui.inputcv);
    ku_window_activate(ui.window, ui.inputtv, 0);
    vh_textinput_activate(ui.inputtv, 0);
}

/* events from all tables */

void ui_on_table_event(vh_table_event_t event)
{
    if (strcmp(event.view->id, "filetable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_FIELDS_UPDATE)
	{
	    /* TODO : save field order */
	}
	else if (event.id == VH_TABLE_EVENT_SELECT && event.ev.repeat == 0)
	{
	    /* show file info and open immediately if possible on simple select */

	    ui_open_file(event.selected_items->data[0]);
	    ui_show_info(event.selected_items->data[0]);

	    ui.rowview_for_context_menu = event.rowview;
	}
	else if (event.id == VH_TABLE_EVENT_OPEN)
	{
	    /* open folder on enter/double click*/

	    ui_open_folder(event.selected_items->data[0]);
	}
	else if (event.id == VH_TABLE_EVENT_DRAG)
	{
	    /* start dragging selected files */

	    ui.dragdatav = event.selected_items;

	    ui_show_drag_view();
	}
	else if (event.id == VH_TABLE_EVENT_CONTEXT)
	{
	    /* show context menu on right click */
	    if (event.rowview)
	    {
		ui.rowview_for_context_menu = event.rowview;
		ui_show_context_menu(event.ev.x, event.ev.y);
	    }
	}
    }
    else if (strcmp(event.view->id, "cliptable") == 0)
    {
	if (event.id == VH_TABLE_EVENT_FIELDS_UPDATE)
	{
	    /* TODO save field config */
	}
	else if (event.id == VH_TABLE_EVENT_SELECT)
	{
	    /* show file info and open immediately if possible on simple select */

	    ui_open_file(event.selected_items->data[0]);
	    ui_show_info(event.selected_items->data[0]);
	}
	else if (event.id == VH_TABLE_EVENT_OPEN)
	{
	    /* open folder on enter/double click*/

	    ui_open_folder(event.selected_items->data[0]);
	}
	else if (event.id == VH_TABLE_EVENT_DROP)
	{
	    /* add dragged data to clipboard table */

	    if (ui.dragdatav)
	    {
		for (int index = 0; index < ui.dragdatav->length; index++)
		{
		    mt_map_t* file = ui.dragdatav->data[index];
		    mt_vector_add_unique_data(ui.clipdatav, file);
		}

		vh_table_set_data(ui.cliptablev, ui.clipdatav);
	    }
	}
    }
    else if (strcmp(event.view->id, "contexttable") == 0)
    {
	/* remove context popup immediately */

	if (event.ev.type == KU_EVENT_KEY_UP && event.ev.keycode == XKB_KEY_Escape)
	{
	    if (ui.contextcv->parent)
	    {
		ku_view_remove_from_parent(ui.contextcv);
		ku_view_t* contexttableevt = GETV(ui.contextcv, "contexttable_event");
		ku_window_activate(ui.window, contexttableevt, 0);
	    }
	}

	if ((event.ev.type == KU_EVENT_MOUSE_DOWN && event.id == VH_TABLE_EVENT_SELECT) ||
	    (event.ev.type == KU_EVENT_KEY_DOWN && event.id == VH_TABLE_EVENT_OPEN))
	{
	    if (ui.contextcv->parent)
	    {
		ku_view_remove_from_parent(ui.contextcv);
		ku_view_t* contexttableevt = GETV(ui.contextcv, "contexttable_event");
		ku_window_activate(ui.window, contexttableevt, 0);
	    }

	    if (event.selected_index == 0)
	    {
		/* create folder */
		ui.inputmode             = UI_IM_NEWFOLDER;
		ku_view_t* filetablebody = GETV(ui.basev, "filetable");

		ku_rect_t rframe = filetablebody->frame.global;

		ui_show_input_popup(rframe.x, rframe.y - 5, "new folder");
	    }
	    else if (event.selected_index == 1)
	    {
		/* rename, open input popup over rowview */

		if (event.rowview)
		{
		    ui.inputmode     = UI_IM_RENAME;
		    ku_rect_t rframe = ui.rowview_for_context_menu->frame.global;

		    vh_table_t* vh = ui.filetablev->handler_data;

		    mt_map_t* info  = vh->selected_items->data[0];
		    char*     value = MGET(info, "file/basename");

		    ui_show_input_popup(rframe.x, rframe.y - 5, value ? value : "");
		}
	    }
	    else if (event.selected_index == 2)
	    {
		/* delete selected items from file table */

		ui_open_approve_popup();
	    }
	    else if (event.selected_index == 3)
	    {
		/* send items to clipboard table */
		vh_table_t* vh = ui.filetablev->handler_data;
		for (int index = 0; index < vh->selected_items->length; index++)
		{
		    mt_map_t* file = vh->selected_items->data[index];
		    mt_vector_add_unique_data(ui.clipdatav, file);
		}

		vh_table_set_data(ui.cliptablev, ui.clipdatav);
	    }
	    else if (event.selected_index == 4)
	    {
		/* paste items in clipboard table using copy */

		if (ui.current_folder)
		{
		    for (int index = 0; index < ui.clipdatav->length; index++)
		    {
			mt_map_t* data    = ui.clipdatav->data[index];
			char*     path    = MGET(data, "file/path");
			char*     name    = MGET(data, "file/basename");
			char*     newpath = STRNF(PATH_MAX, "%s/%s", ui.current_folder, name);

			int res = fm_copy(path, newpath);

			if (res < 0)
			{
			    ui_show_status("Copy error");
			}
			else
			{
			    ui_show_status("File %s copied to %s", name, newpath);
			}
		    }
		    ui_load_folder(ui.current_folder);
		}
	    }
	    else if (event.selected_index == 5)
	    {
		/* paste items in clipboard table using move */

		for (int index = 0; index < ui.clipdatav->length; index++)
		{
		    mt_map_t* data    = ui.clipdatav->data[index];
		    char*     path    = MGET(data, "file/path");
		    char*     name    = MGET(data, "file/basename");
		    char*     newpath = STRNF(PATH_MAX, "%s/%s", ui.current_folder, name);

		    int res = fm_rename1(path, newpath);

		    if (res < 0)
		    {
			ui_show_status("Move error");
		    }
		    else
		    {
			ui_show_status("File %s moved to %s", name, newpath);
		    }
		}
		ui_load_folder(ui.current_folder);
		mt_vector_reset(ui.clipdatav);
		vh_table_set_data(ui.cliptablev, ui.clipdatav);
	    }
	    else if (event.selected_index == 6)
	    {
		/* reset clipboard */

		mt_vector_reset(ui.clipdatav);
		vh_table_set_data(ui.cliptablev, ui.clipdatav);
	    }
	}
    }
}

/* content view event, click at the moment */

void on_cv_event(vh_cv_evnt_event_t event)
{
    if (event.id == VH_CV_EVENT_CLICK)
    {
	/* load next document page or play/pause */

	if (ui.media_type == UI_MT_DOCUMENT) ui_show_pdf_page(ui.pdf_page + 1);
	else if (ui.coder_type == CODER_MEDIA_TYPE_VIDEO || ui.coder_type == CODER_MEDIA_TYPE_AUDIO) ui_toggle_pause();
    }
}

/* touch events from views */

void ui_on_touch(vh_touch_event_t event)
{
    if (strcmp(event.view->id, "inputcont") == 0) ui_cancel_input();
    else if (strcmp(event.view->id, "contextpopupcont") == 0) ku_view_remove_from_parent(ui.contextcv);
}

/* global key events */

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

    if (event.ev.keycode == XKB_KEY_Delete) ui_open_approve_popup();
    if (event.ev.keycode == XKB_KEY_d && event.ev.ctrl_down) ui_open_approve_popup();

    /* TODO should move these to the okaycv popup or its buttons */
    if (event.ev.keycode == XKB_KEY_Return && ui.okaycv->parent)
    {
	ku_view_remove_from_parent(ui.okaycv);
	ui_delete_selected_files();
    }

    if (event.ev.keycode == XKB_KEY_Escape)
    {
	if (ui.okaycv->parent) ku_view_remove_from_parent(ui.okaycv);
	if (ui.contextcv->parent) ku_view_remove_from_parent(ui.contextcv);
	if (ui.settingscv->parent) ku_view_remove_from_parent(ui.settingscv);
    }

    if (event.ev.keycode == XKB_KEY_c && event.ev.ctrl_down)
    {
	vh_table_t* vh = ui.filetablev->handler_data;
	for (int index = 0; index < vh->selected_items->length; index++)
	{
	    vh_table_t* vh   = ui.filetablev->handler_data;
	    mt_map_t*   file = vh->selected_items->data[index];
	    mt_vector_add_unique_data(ui.clipdatav, file);
	}

	vh_table_set_data(ui.cliptablev, ui.clipdatav);
    }

    if (event.ev.keycode == XKB_KEY_v && event.ev.ctrl_down)
    {
	if (ui.rowview_for_context_menu)
	{
	    ku_rect_t frame = ui.rowview_for_context_menu->frame.global;
	    ui_show_context_menu(frame.x, frame.y);
	}
    }
    if (event.ev.keycode == XKB_KEY_s && event.ev.ctrl_down)
    {
	ui_rotate_sidebar();
    }
    if (event.ev.keycode == XKB_KEY_r && event.ev.ctrl_down)
    {
	ui.inputmode     = UI_IM_RENAME;
	ku_rect_t rframe = ui.rowview_for_context_menu->frame.global;

	vh_table_t* vh    = ui.filetablev->handler_data;
	mt_map_t*   info  = vh->selected_items->data[0];
	char*       value = MGET(info, "file/basename");

	ui_show_input_popup(rframe.x, rframe.y - 5, value ? value : "");
    }
    if (event.ev.keycode == XKB_KEY_n && event.ev.ctrl_down)
    {
	ui.inputmode = UI_IM_NEWFOLDER;

	ku_view_t* filetablebody = GETV(ui.basev, "filetable");
	ku_rect_t  rframe        = filetablebody->frame.global;

	ui_show_input_popup(rframe.x, rframe.y - 5, "new folder");
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

/* button events */

void ui_on_btn_event(vh_button_event_t event)
{
    if (strcmp(event.view->id, "playbtn") == 0)
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
    else if (strcmp(event.view->id, "prevbtn") == 0)
    {
	ui_show_pdf_page(ui.pdf_page - 1);
    }
    else if (strcmp(event.view->id, "nextbtn") == 0)
    {
	ui_show_pdf_page(ui.pdf_page + 1);
    }
    else if (strcmp(event.view->id, "plusbtn") == 0)
    {
	ku_view_t* evview = ku_view_get_subview(ui.basev, "previewevt");
	vh_cv_evnt_zoom(evview, 0.1);
    }
    else if (strcmp(event.view->id, "minusbtn") == 0)
    {
	ku_view_t* evview = ku_view_get_subview(ui.basev, "previewevt");
	vh_cv_evnt_zoom(evview, -0.1);
    }
    else if (strcmp(event.view->id, "settingsclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.settingscv);
    }
    else if (strcmp(event.view->id, "pathclearbtn") == 0)
    {
	vh_textinput_set_text(ui.pathtv, "");
	ku_window_activate(ui.window, ui.pathtv, 1);
	vh_textinput_activate(ui.pathtv, 1);
    }
    else if (strcmp(event.view->id, "okayacceptbtn") == 0)
    {
	ku_view_remove_from_parent(ui.okaycv);
	ui_delete_selected_files();
    }
    else if (strcmp(event.view->id, "okayclosebtn") == 0)
    {
	ku_view_remove_from_parent(ui.okaycv);
    }
    else if (strcmp(event.view->id, "closebtn") == 0)
    {
	ku_wayland_exit();
    }
    else if (strcmp(event.view->id, "maxbtn") == 0)
    {
	ku_wayland_toggle_fullscreen();
    }
    else if (strcmp(event.view->id, "sidebarbtn") == 0)
    {
	ui_rotate_sidebar();
    }
    else if (strcmp(event.view->id, "settingsbtn") == 0)
    {
	if (!ui.settingscv->parent)
	{
	    ku_view_add_subview(ui.basev, ui.settingscv);
	    ku_view_layout(ui.basev, ui.basev->style.scale);
	}
    }
}

/* slider event, comes from seekbar currently */

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
	ui_show_pdf_page(new_page);
    }
}

/* text edition events from text fields */

void ui_on_text_event(vh_textinput_event_t event)
{
    if (strcmp(event.view->id, "inputtf") == 0)
    {
	if (event.id == VH_TEXTINPUT_TEXT)
	{
	    // resize input back if needed
	    ku_rect_t inputrect = ui.inputtv->frame.local;
	    ku_rect_t backrect  = ui.inputbckv->frame.local;

	    if (inputrect.h > backrect.h)
	    {
		backrect.h = inputrect.h + 5;
		ku_view_set_frame(ui.inputbckv, backrect);
	    }
	}
	if (event.id == VH_TEXTINPUT_DEACTIVATE) ui_cancel_input();
	if (event.id == VH_TEXTINPUT_RETURN)
	{
	    ui_cancel_input();

	    if (ui.inputmode == UI_IM_RENAME)
	    {
		vh_table_t* vh = ui.filetablev->handler_data;
		if (vh->selected_items->length > 0)
		{
		    mt_map_t* file   = vh->selected_items->data[0];
		    char*     parent = MGET(file, "file/parent");

		    char* oldpath = MGET(file, "file/path");
		    char* newpath = mt_path_new_append(parent, event.text);

		    fm_rename(oldpath, newpath, NULL);

		    REL(newpath);

		    ui_load_folder(ui.current_folder);
		}
	    }
	    if (ui.inputmode == UI_IM_NEWFOLDER)
	    {
		char* newpath = mt_path_new_append(ui.current_folder, event.text);
		char* folpath = mt_path_new_append(newpath, "/");

		int res = fm_create(folpath, 0777);

		if (res == 0)
		{
		    if (ui.current_folder) MPUT(ui.folder_history, ui.current_folder, newpath);

		    ui_load_folder(ui.current_folder);
		}
		else ui_show_status("Folder creation error");

		REL(newpath);
		REL(folpath);
	    }
	}
    }

    else if (strcmp(event.view->id, "pathtf") == 0)
    {
	if (event.id == VH_TEXTINPUT_DEACTIVATE) vh_textinput_activate(ui.pathtv, 0);
	if (event.id == VH_TEXTINPUT_RETURN)
	{
	    vh_textinput_activate(ui.pathtv, 0);
	    ku_window_activate(ui.window, ui.pathtv, 0);

	    char* valid_path = STRNC(event.text);
	    for (;;)
	    {
		if (fm_exists(valid_path)) break;
		else
		{
		    char* prev_path = mt_path_new_remove_last_component(valid_path);
		    REL(valid_path);
		    valid_path = prev_path;
		}
	    }
	    ui_load_folder(valid_path);
	    REL(valid_path);

	    /* activate file list */
	    ku_view_t* filetableevnt = GETV(ui.basev, "filetable_event");
	    ku_window_activate(ui.window, filetableevnt, 1);
	}
    }
}

/* drag layer events */

void ui_on_drag(vh_drag_event_t event)
{
    if (event.id == VH_DRAG_DROP)
	if (ui.draggedv) ku_view_remove_from_parent(ui.draggedv);
}

void ui_init(int width, int height, float scale, ku_window_t* window)
{
    ku_text_init();

    ui.window     = window;
    ui.play_state = 1; /* TODO load it from config */

    ui.filedatav      = VNEW();
    ui.clipdatav      = VNEW();
    ui.folder_history = MNEW();

    /* generate views from descriptors */

    mt_vector_t* view_list = VNEW();

    ku_gen_html_parse(config_get("html_path"), view_list);
    ku_gen_css_apply(view_list, config_get("css_path"), config_get("img_path"));
    ku_gen_type_apply(view_list, ui_on_btn_event, ui_on_slider_event);

    ku_view_t* bv = mt_vector_head(view_list);

    ui.basev = RET(bv);
    ku_window_add(ui.window, ui.basev);
    ku_window_activate(ui.window, ui.basev, 1);

    REL(view_list);
    /* listen for keys and shortcuts */

    vh_key_add(ui.basev, ui_on_key_down);
    ui.basev->needs_key = 1;

    /* setup drag layer */

    ui.dragv = GETV(bv, "draglayer");
    vh_drag_attach(ui.dragv, ui_on_drag);

    /* setup visualizer */

    ui.prevcontv = GETV(bv, "previewcont");
    ui.prevbodyv = GETV(bv, "previewbody");

    /* files table */

    mt_vector_t* fields = VNEW();
    VADDR(fields, STRNC("file/basename"));
    VADDR(fields, mt_number_new_int(400));
    VADDR(fields, STRNC("file/size"));
    VADDR(fields, mt_number_new_int(120));
    VADDR(fields, STRNC("file/last_access"));
    VADDR(fields, mt_number_new_int(180));
    VADDR(fields, STRNC("file/last_modification"));
    VADDR(fields, mt_number_new_int(180));
    VADDR(fields, STRNC("file/last_status"));
    VADDR(fields, mt_number_new_int(180));

    ui.filetablev = GETV(bv, "filetable");
    vh_table_attach(ui.filetablev, fields, ui_on_table_event);

    /* clipboard table */

    ui.cliptablev = GETV(bv, "cliptable");
    vh_table_attach(ui.cliptablev, fields, ui_on_table_event);

    REL(fields);

    /* file info table */

    fields = VNEW();

    VADDR(fields, STRNC("key"));
    VADDR(fields, mt_number_new_int(150));
    VADDR(fields, STRNC("value"));
    VADDR(fields, mt_number_new_int(400));

    ui.infotablev = GETV(bv, "infotable");
    vh_table_attach(ui.infotablev, fields, ui_on_table_event);

    REL(fields);

    /* context popup table */

    fields = VNEW();

    VADDR(fields, STRNC("value"));
    VADDR(fields, mt_number_new_int(200));

    ui.contexttablev = GETV(bv, "contexttable");
    vh_table_attach(ui.contexttablev, fields, ui_on_table_event);

    vh_table_t* table = ui.contexttablev->handler_data;

    /* hack for context menu popup animation */
    table->layr_v->style.masked = 0;

    REL(fields);

    mt_vector_t* items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Create folder")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Rename")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Delete")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Send to clipboard")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Paste using copy")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Paste using move")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(50, "Reset clipboard")}));

    vh_table_set_data(ui.contexttablev, items);
    REL(items);

    /* settings table */

    fields = VNEW();

    VADDR(fields, STRNC("value"));
    VADDR(fields, mt_number_new_int(510));

    ui.settingstablev = GETV(bv, "settingstable");
    vh_table_attach(ui.settingstablev, fields, ui_on_table_event);

    REL(fields);

    items = VNEW();

    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "MultiMedia File Manager v%s beta by Milan Toth", MMFM_VERSION)}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Free and Open Source Software")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Donate on Paypal")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Toggle play/pause : SPACE")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Zoom in/out : + / -")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Switch between tables and input fields : TAB")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Context menu/Paste files : CTRL+V")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Send file to clipboard : CTRL+C")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Create folder : CTRL+N")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Rename file : CTRL+R")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Delete file : CTRL+D")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Toggle info view/clipboard : CTRL+S")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Cancel input : ESC")}));
    VADDR(items, mapu_pair((mpair_t){"value", STRNF(200, "Cancel popup : ESC")}));

    vh_table_set_data(ui.settingstablev, items);
    REL(items);

    /* preview block */

    ku_view_t* preview     = GETV(bv, "preview");
    ku_view_t* previewbody = GETV(bv, "previewbody");
    ku_view_t* previewscrl = GETV(bv, "previewscrl");
    ku_view_t* previewevnt = GETV(bv, "previewevt");
    ku_view_t* previewcont = GETV(bv, "previewcont");

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
	mt_log_debug("preview not found");

    /* get main bottom for layout change */

    ui.mainbottomv = GETV(bv, "main_bottom");

    /* settings popup */

    ku_view_t* settingscv    = GETV(bv, "settingspopupcont");
    ku_view_t* settingspopup = GETV(bv, "settingspopup");

    settingspopup->blocks_touch  = 1;
    settingspopup->blocks_scroll = 1;

    ui.settingscv = RET(settingscv);
    ku_view_remove_from_parent(ui.settingscv);

    /* context popup */

    ku_view_t* contextcv    = GETV(bv, "contextpopupcont");
    ku_view_t* contextpopup = GETV(bv, "contextpopup");
    ku_view_t* contextanimv = GETV(bv, "contextpopupanim");

    vh_anim_add(contextanimv, NULL, NULL);

    contextpopup->blocks_touch  = 1;
    contextpopup->blocks_scroll = 1;

    ui.contextcv = RET(contextcv);

    vh_anim_add(contextpopup, NULL, NULL);

    vh_touch_add(ui.contextcv, ui_on_touch);
    ku_view_remove_from_parent(ui.contextcv);

    /* okay popup */

    ku_view_t* okaycv    = GETV(bv, "okaypopupcont");
    ku_view_t* okaypopup = GETV(bv, "okaypopup");

    okaypopup->blocks_key    = 1;
    okaypopup->blocks_touch  = 1;
    okaypopup->blocks_scroll = 1;

    ui.okaycv    = RET(okaycv);
    ui.okaypopup = RET(okaypopup);

    vh_key_add(okaypopup, ui_on_key_down);
    vh_touch_add(ui.okaycv, ui_on_touch);
    ku_view_remove_from_parent(ui.okaycv);

    /* status bar */

    ui.statuslv = GETV(bv, "statustf");

    /* input popup */

    ui.inputcv   = RET(GETV(bv, "inputcont"));
    ui.inputbckv = GETV(bv, "inputbck");
    ui.inputtv   = GETV(bv, "inputtf");

    ui.inputbckv->blocks_touch = 1;
    ui.inputcv->blocks_touch   = 1;
    ui.inputcv->blocks_scroll  = 1;

    vh_textinput_add(ui.inputtv, "Generic input", "", ui_on_text_event);
    vh_touch_add(ui.inputcv, ui_on_touch);
    ku_view_remove_from_parent(ui.inputcv);

    /* path bar */

    ui.pathtv = GETV(bv, "pathtf");
    vh_textinput_add(ui.pathtv, "/home/milgra", "", ui_on_text_event);

    /* time label */

    ui.seeklv   = GETV(bv, "seektf");
    ui.seekbarv = GETV(bv, "seekbar");

    /* main gap */

    ui.cliptablev = RET(GETV(bv, "cliptable"));
    ui.infotablev = RET(GETV(bv, "infotable"));

    ui.playbtnv  = RET(GETV(bv, "playbtn"));
    ui.prevbtnv  = RET(GETV(bv, "prevbtn"));
    ui.nextbtnv  = RET(GETV(bv, "nextbtn"));
    ui.plusbtnv  = RET(GETV(bv, "plusbtn"));
    ui.minusbtnv = RET(GETV(bv, "minusbtn"));

    ui_update_control_bar();

    /* set up focusable */

    ku_view_t* filetableevnt   = GETV(ui.filetablev, "filetable_event");
    ku_view_t* infotableevt    = GETV(ui.infotablev, "infotable_event");
    ku_view_t* cliptableevt    = GETV(ui.cliptablev, "cliptable_event");
    ku_view_t* contexttableevt = GETV(ui.contexttablev, "contexttable_event");
    ku_view_t* previewevt      = GETV(bv, "previewevt");
    ku_view_t* pathtv          = GETV(bv, "pathtf");

    ui.focusable_filelist = VNEW();
    ui.focusable_infolist = VNEW();
    ui.focusable_cliplist = VNEW();

    VADD(ui.focusable_filelist, filetableevnt);
    VADD(ui.focusable_filelist, previewevt);
    VADD(ui.focusable_filelist, pathtv);

    VADD(ui.focusable_infolist, filetableevnt);
    VADD(ui.focusable_infolist, infotableevt);
    VADD(ui.focusable_infolist, previewevt);
    VADD(ui.focusable_infolist, pathtv);

    VADD(ui.focusable_cliplist, filetableevnt);
    VADD(ui.focusable_cliplist, cliptableevt);
    VADD(ui.focusable_cliplist, previewevt);
    VADD(ui.focusable_cliplist, pathtv);

    contexttableevt->blocks_key = 1;

    ku_window_activate(ui.window, filetableevnt, 1); /* start with file list listening for key up and down */

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

    // show texture map for debug

    /* ku_view_t* texmap    = ku_view_new("texmap", ((ku_rect_t){0, 0, 150, 150})); */
    /* texmap->needs_touch  = 0; */
    /* texmap->texture.full = 1; */
    /* texmap->style.right  = 0; */
    /* texmap->style.top    = 0; */

    /* ku_window_add(ui.window, texmap); */
}

void ui_destroy()
{
    if (ui.media_state)
    {
	mp_close(ui.media_state);
	ui.media_state = NULL;
    }

    if (ui.pdf_path) REL(ui.pdf_path);

    REL(ui.focusable_filelist);
    REL(ui.focusable_infolist);
    REL(ui.focusable_cliplist);

    REL(ui.folder_history);
    REL(ui.current_folder);

    REL(ui.playbtnv);
    REL(ui.prevbtnv);
    REL(ui.nextbtnv);
    REL(ui.plusbtnv);
    REL(ui.minusbtnv);

    REL(ui.settingscv);
    REL(ui.contextcv);
    REL(ui.inputcv);
    REL(ui.okaycv);

    REL(ui.filedatav);
    REL(ui.clipdatav);

    REL(ui.basev);

    ku_window_remove(ui.window, ui.basev);

    ku_text_destroy();
    ku_fontconfig_delete();
}

#endif

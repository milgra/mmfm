#ifndef ui_h
#define ui_h

#include "view.c"

void ui_init(float width, float height);
void ui_destroy();
void ui_add_cursor();
void ui_update_cursor(r2_t frame);
void ui_update_dragger();
void ui_render_without_cursor(uint32_t time);
void ui_save_screenshot(uint32_t time, char hide_cursor);
void ui_update_layout(float w, float h);
void ui_describe();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "bm_rgba_util.c"
#include "coder.c"
#include "config.c"
#include "filemanager.c"
#include "tg_css.c"
#include "tg_scaledimg.c"
#include "tg_text.c"
#include "ui_compositor.c"
#include "ui_manager.c"
#include "ui_table.c"
#include "ui_util.c"
#include "ui_visualizer.c"
#include "vh_button.c"
#include "vh_cv_body.c"
#include "vh_cv_evnt.c"
#include "vh_cv_scrl.c"
#include "vh_drag.c"
#include "vh_key.c"
#include "vh_touch.c"
#include "view_layout.c"
#include "viewgen_css.c"
#include "viewgen_html.c"
#include "viewgen_type.c"
#include "wm_connector.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_number.c"
#include "zc_path.c"
#include "zc_time.c"
#include <limits.h>

struct _ui_t
{
    view_t* view_base;
    view_t* view_drag; // drag overlay
    view_t* view_doc;  //  file drag icon
    view_t* cursor;    // replay cursor
    view_t* view_infotf;
    view_t* view_maingap;

    view_t* cliplistbox;
    view_t* fileinfobox;

    view_t* exit_btn;
    view_t* full_btn;
    view_t* clip_btn;
    view_t* prop_btn;

    view_t* main_bottom;
    view_t* left_dragger;
    view_t* prev_dragger;

    vec_t* file_list_data;
    vec_t* clip_list_data;
    vec_t* drag_data;

    ui_table_t* cliptable;
    ui_table_t* filelisttable;
    ui_table_t* fileinfotable;

    textstyle_t progress_style;
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

void on_files_event(ui_table_t* table, ui_table_event event, void* userdata)
{
    switch (event)
    {
	case UI_TABLE_EVENT_FIELDS_UPDATE:
	{
	    zc_log_debug("fields update %s", table->id);
	}
	break;
	case UI_TABLE_EVENT_SELECT:
	{
	    zc_log_debug("select %s", table->id);

	    vec_t* selected = userdata;
	    map_t* info     = selected->data[0];
	    char*  path     = MGET(info, "file/path");

	    ui_visualizer_open(path);

	    // ask fore detailed info if needed

	    if (!MGET(info, "file/mime")) fm_detail(info);

	    vec_t* keys = VNEW(); // REL L0
	    map_keys(info, keys);
	    vec_sort(keys, VSD_ASC, ((int (*)(void*, void*)) strcmp));

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

	    ui_table_set_data(ui.fileinfotable, items);

	    ui_show_progress("File info/data loaded");

	    REL(items); // REL L1
	}
	break;
	case UI_TABLE_EVENT_OPEN:
	{
	    zc_log_debug("open %s", table->id);

	    vec_t* selected = userdata;
	    map_t* info     = selected->data[0];

	    char* type = MGET(info, "file/type");
	    char* path = MGET(info, "file/path");

	    if (strcmp(type, "directory") == 0)
	    {
		map_t* files = MNEW(); // REL 0
		zc_time(NULL);
		fm_list(path, files);
		zc_time("file list");
		vec_reset(ui.file_list_data);
		map_values(files, ui.file_list_data);
		vec_sort(ui.file_list_data, VSD_ASC, ui_comp_entry);
		ui_table_set_data(ui.filelisttable, ui.file_list_data);
		REL(files);

		ui_show_progress("Directory loaded");
	    }
	    else
	    {
		ui_visualizer_open(path);
	    }
	}
	break;
	case UI_TABLE_EVENT_DRAG:
	{
	    vec_t*  selected                = userdata;
	    view_t* docview                 = view_new("dragged_view", ((r2_t){.x = 0, .y = 0, .w = 50, .h = 50}));
	    char*   imagepath               = cstr_new_format(100, "%s/img/%s", config_get("res_path"), "freebsd.png");
	    docview->style.background_image = imagepath;
	    tg_css_add(docview);

	    view_insert_subview(ui.view_base, docview, ui.view_base->views->length - 2);

	    vh_drag_drag(ui.view_drag, docview);
	    REL(docview);

	    ui.drag_data = selected;
	    ui.view_doc  = docview;
	}
	break;
	case UI_TABLE_EVENT_DROP:
	    break;
    }
}

void on_clipboard_event(ui_table_t* table, ui_table_event event, void* userdata)
{
    switch (event)
    {
	case UI_TABLE_EVENT_FIELDS_UPDATE:
	{
	    zc_log_debug("fields update %s", table->id);
	}
	break;
	case UI_TABLE_EVENT_SELECT:
	{
	    zc_log_debug("select %s", table->id);

	    vec_t* selected = userdata;
	    map_t* info     = selected->data[0];

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

	    ui_table_set_data(ui.fileinfotable, items);

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
	}
	break;
	case UI_TABLE_EVENT_DRAG:
	    break;
	case UI_TABLE_EVENT_DROP:
	{
	    if (ui.drag_data)
	    {
		size_t index = (size_t) userdata;
		zc_log_debug("DROP %i %i", index, ui.drag_data->length);
		vec_add_in_vector(ui.clip_list_data, ui.drag_data);
		ui.drag_data = NULL;
		ui_table_set_data(ui.cliptable, ui.clip_list_data);

		if (ui.view_doc)
		{
		    view_remove_from_parent(ui.view_doc);
		}
	    }
	}
	break;
	default:
	    break;
    }
}

void on_fileinfo_event(ui_table_t* table, ui_table_event event, void* userdata)
{
}

void ui_on_key_down(void* userdata, void* data)
{
    // ev_t* ev = (ev_t*) data;
}

void ui_on_btn_event(void* userdata, void* data)
{
    // ev_t* ev = (ev_t*) data;
    view_t* btnview = data;

    if (btnview == ui.exit_btn) wm_close();
    if (btnview == ui.full_btn) wm_toggle_fullscreen();

    if (btnview == ui.clip_btn)
    {
	view_t* top = view_get_subview(ui.view_base, "top_container");
	if (ui.fileinfobox->parent) view_remove_from_parent(ui.fileinfobox);

	if (ui.cliplistbox->parent)
	{
	    view_remove_from_parent(ui.cliplistbox);
	}
	else
	{
	    view_add_subview(top, ui.cliplistbox);
	}
	view_layout(top);
    }
    if (btnview == ui.prop_btn)
    {
	view_t* top = view_get_subview(ui.view_base, "top_container");
	if (ui.cliplistbox->parent) view_remove_from_parent(ui.cliplistbox);

	if (ui.fileinfobox->parent)
	{
	    view_remove_from_parent(ui.fileinfobox);
	}
	else
	{
	    view_add_subview(top, ui.fileinfobox);
	}
	view_layout(top);
    }

    if (btnview == ui.left_dragger)
    {
	vh_drag_drag(ui.view_drag, ui.left_dragger);
    }
    if (btnview == ui.prev_dragger)
    {
	vh_drag_drag(ui.view_drag, ui.prev_dragger);
    }
}

void ui_on_drag_move(void* userdata, void* data)
{
    view_t* v = userdata;
}

void ui_on_drag_drop(void* userdata, void* data)
{
    view_t* v = data;

    if (v == ui.left_dragger)
    {
	view_t* cbox       = view_get_subview(ui.view_base, "cliplistbox");
	view_t* fbox       = view_get_subview(ui.view_base, "fileinfobox");
	view_t* lft        = view_get_subview(ui.view_base, "left_container");
	view_t* top        = view_get_subview(ui.view_base, "top_container");
	cbox->style.height = lft->frame.global.h - v->frame.global.y + 28.0;
	fbox->style.width  = top->frame.global.w - v->frame.global.x - 9;
	zc_log_debug("height %i width %i", cbox->style.height, fbox->style.width);
	view_layout(top);
    }
}

void ui_add_cursor()
{
    ui.cursor                         = view_new("ui.cursor", ((r2_t){10, 10, 10, 10}));
    ui.cursor->exclude                = 0;
    ui.cursor->style.background_color = 0xFF000099;
    ui.cursor->needs_touch            = 0;
    tg_css_add(ui.cursor);
    ui_manager_add_to_top(ui.cursor);
}

void ui_update_cursor(r2_t frame)
{
    view_set_frame(ui.cursor, frame);
}

void ui_render_without_cursor(uint32_t time)
{
    ui_manager_remove(ui.cursor);
    ui_manager_render(time);
    ui_manager_add_to_top(ui.cursor);
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

void ui_create_views(float width, float height)
{
    // generate views from descriptors

    vec_t* view_list = viewgen_html_create(config_get("html_path"));
    viewgen_css_apply(view_list, config_get("css_path"), config_get("res_path"));
    viewgen_type_apply(view_list);
    ui.view_base = RET(vec_head(view_list));
    REL(view_list);

    // initial layout of views

    view_set_frame(ui.view_base, (r2_t){0.0, 0.0, (float) width, (float) height});
    view_layout(ui.view_base);
    ui_manager_add(ui.view_base);
}

int ui_comp_text(void* left, void* right)
{
    char* la = left;
    char* ra = right;

    return strcmp(la, ra);
}

void ui_init(float width, float height)
{
    text_init();                    // DESTROY 0
    ui_manager_init(width, height); // DESTROY 1
    ui_create_views(width, height);

    // setup key events

    cb_t* key_cb = cb_new(ui_on_key_down, NULL);
    vh_key_add(ui.view_base, key_cb); // listen on ui.view_base for shortcuts
    REL(key_cb);

    // setup drag layer

    cb_t* move_cb = cb_new(ui_on_drag_move, NULL);
    cb_t* drop_cb = cb_new(ui_on_drag_drop, NULL);

    ui.view_drag = view_get_subview(ui.view_base, "draglayer");
    vh_drag_attach(ui.view_drag, move_cb, drop_cb);

    REL(move_cb);
    REL(drop_cb);

    /* setup visualizer */

    ui_visualizer_attach(ui.view_base); // DETACH 8

    /* tables */

    textstyle_t ts  = {0};
    ts.font         = config_get("font_path");
    ts.align        = TA_CENTER;
    ts.margin_right = 0;
    ts.size         = 60.0;
    ts.textcolor    = 0x353535FF;
    ts.backcolor    = 0x252525FF;
    ts.multiline    = 0;

    /* preview block */

    view_t* preview     = view_get_subview(ui.view_base, "preview");
    view_t* previewbody = view_get_subview(ui.view_base, "previewbody");
    view_t* previewscrl = view_get_subview(ui.view_base, "previewscrl");
    view_t* previewevnt = view_get_subview(ui.view_base, "previewevnt");
    view_t* previewcont = view_get_subview(ui.view_base, "previewcont");

    if (preview)
    {
	tg_text_add(preview);
	/* ts.backcolor = 0x343434FF; */
	tg_text_set(preview, "PREVIEW", ts);
	/* ts.backcolor = 0x252525FF; */

	tg_scaledimg_add(previewcont, 300, 300);
	view_set_frame(previewcont, (r2_t){0, 0, 300, 300});
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

    view_t* fileinfo       = view_get_subview(ui.view_base, "fileinfotable");
    view_t* fileinfoscroll = view_get_subview(ui.view_base, "fileinfoscroll");
    view_t* fileinfoevt    = view_get_subview(ui.view_base, "fileinfoevt");
    view_t* fileinfohead   = view_get_subview(ui.view_base, "fileinfohead");

    if (fileinfo)
    {
	tg_text_add(fileinfo);
	tg_text_set(fileinfo, "FILE INFO", ts);

	ui.fileinfotable = ui_table_create(
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

    view_t* filelist       = view_get_subview(ui.view_base, "filelisttable");
    view_t* filelistscroll = view_get_subview(ui.view_base, "filelistscroll");
    view_t* filelistevt    = view_get_subview(ui.view_base, "filelistevt");
    view_t* filelisthead   = view_get_subview(ui.view_base, "filelisthead");

    if (filelist)
    {
	tg_text_add(filelist);
	tg_text_set(filelist, "FILES", ts);
    }
    else
	zc_log_debug("filelistbck not found");

    ui.filelisttable = ui_table_create(
	"filelisttable",
	filelist,
	filelistscroll,
	filelistevt,
	filelisthead,
	fields,
	on_files_event);

    ui.file_list_data = VNEW(); // REL S0
    ui_table_set_data(ui.filelisttable, ui.file_list_data);

    // clipboard table

    view_t* cliplist       = view_get_subview(ui.view_base, "cliplist");
    view_t* cliplistscroll = view_get_subview(ui.view_base, "cliplistscroll");
    view_t* cliplistevt    = view_get_subview(ui.view_base, "cliplistevt");
    view_t* cliplisthead   = view_get_subview(ui.view_base, "cliplisthead");

    if (cliplist)
    {
	tg_text_add(cliplist);
	tg_text_set(cliplist, "CLIPBOARD", ts);

	ui.cliptable = ui_table_create(
	    "cliptable",
	    cliplist,
	    cliplistscroll,
	    cliplistevt,
	    cliplisthead,
	    fields,
	    on_clipboard_event);

	ui.clip_list_data = VNEW();
	ui_table_set_data(ui.cliptable, ui.clip_list_data);
    }
    else
	zc_log_debug("cliplistbck not found");

    REL(fields);

    // fill up files table

    map_t* files = MNEW(); // REL 0
    fm_list(config_get("top_path"), files);
    map_values(files, ui.file_list_data);
    REL(files);

    vec_sort(ui.file_list_data, VSD_ASC, ui_comp_entry);

    ui_table_set_data(ui.filelisttable, ui.file_list_data);

    /* close button */

    cb_t* btn_cb = cb_new(ui_on_btn_event, NULL);

    ui.exit_btn = view_get_subview(ui.view_base, "app_close_btn");
    vh_button_add(ui.exit_btn, VH_BUTTON_NORMAL, btn_cb);

    ui.full_btn = view_get_subview(ui.view_base, "app_maximize_btn");
    vh_button_add(ui.full_btn, VH_BUTTON_NORMAL, btn_cb);

    ui.clip_btn = view_get_subview(ui.view_base, "show_clipboard_btn");
    vh_button_add(ui.clip_btn, VH_BUTTON_NORMAL, btn_cb);

    ui.prop_btn = view_get_subview(ui.view_base, "show_properties_btn");
    vh_button_add(ui.prop_btn, VH_BUTTON_NORMAL, btn_cb);

    // get main bottom for layout change

    ui.main_bottom                = view_get_subview(ui.view_base, "main_bottom");
    ui.left_dragger               = view_get_subview(ui.view_base, "left_dragger");
    ui.left_dragger->blocks_touch = 1;

    vh_touch_add(ui.left_dragger, btn_cb);

    REL(btn_cb);

    // info textfield

    ui.view_infotf = view_get_subview(ui.view_base, "infotf");
    tg_text_add(ui.view_infotf);

    ui.progress_style = ui_util_gen_textstyle(ui.view_infotf);
    ui_show_progress("Directory loaded");

    // main gap

    ui.view_maingap = view_get_subview(ui.view_base, "main_gap");

    ui.cliplistbox = RET(view_get_subview(ui.view_base, "cliplistbox"));
    ui.fileinfobox = RET(view_get_subview(ui.view_base, "fileinfobox"));

    view_remove_from_parent(ui.cliplistbox);
    view_remove_from_parent(ui.fileinfobox);

    // show texture map for debug

    /* view_t* texmap       = view_new("texmap", ((r2_t){0, 0, 150, 150})); */
    /* texmap->needs_touch  = 0; */
    /* texmap->exclude      = 0; */
    /* texmap->texture.full = 1; */
    /* texmap->style.right  = 0; */
    /* texmap->style.top    = 0; */

    /* ui_manager_add(texmap); */
}

void ui_destroy()
{
    ui_visualizer_detach();

    ui_manager_remove(ui.view_base);

    REL(ui.cliplistbox);
    REL(ui.fileinfobox);

    // TODO create a heap based textstyle object to make this easier
    REL(ui.progress_style.font);

    REL(ui.view_base);
    REL(ui.fileinfotable);
    REL(ui.filelisttable);
    REL(ui.cliptable);

    REL(ui.file_list_data);
    REL(ui.clip_list_data);

    ui_manager_destroy(); // DESTROY 1

    text_destroy(); // DESTROY 0
}

void ui_update_layout(float w, float h)
{
    if (w > h && ui.main_bottom->style.flexdir == FD_COL)
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
}

void ui_update_dragger()
{
    view_t* filelist = view_get_subview(ui.view_base, "filelisttable");
    r2_t    df       = ui.left_dragger->frame.local;
    df.x             = filelist->frame.global.x + filelist->frame.global.w;
    df.y             = filelist->frame.global.y + filelist->frame.global.h;
    view_set_frame(ui.left_dragger, df);
}

void ui_describe()
{
    view_describe(ui.view_base, 0);
}

#endif

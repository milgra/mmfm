#include "coder.c"
#include "config.c"
#include "evrecorder.c"
#include "filemanager.c"
#include "ku_connector_wayland.c"
#include "ku_gl.c"
#include "ku_renderer_egl.c"
#include "ku_renderer_soft.c"
#include "ku_window.c"
#include "mt_bitmap_ext.c"
#include "mt_log.c"
#include "mt_map.c"
#include "mt_path.c"
#include "mt_string.c"
#include "mt_time.c"
#include "ui.c"
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>

struct
{
    char              replay;
    char              record;
    struct wl_window* wlwindow;
    ku_window_t*      kuwindow;

    ku_rect_t    dirtyrect;
    int          softrender;
    mt_vector_t* eventqueue;

    char* rec_path;
    char* rep_path;
} mmfm = {0};

void init(wl_event_t event)
{
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_AUDIO);

    mmfm.eventqueue = VNEW();

    struct monitor_info* monitor = event.monitors[0];

    if (mmfm.softrender)
    {
	mmfm.wlwindow = ku_wayland_create_window("mmfm", 1200, 600);
    }
    else
    {
	mmfm.wlwindow = ku_wayland_create_eglwindow("mmfm", 1200, 600);

	int max_width  = 0;
	int max_height = 0;

	for (int index = 0; index < event.monitor_count; index++)
	{
	    struct monitor_info* monitor = event.monitors[index];
	    if (monitor->logical_width > max_width) max_width = monitor->logical_width;
	    if (monitor->logical_height > max_height) max_height = monitor->logical_height;
	}
	ku_renderer_egl_init(max_width, max_height);
    }

    mmfm.kuwindow = ku_window_create(monitor->logical_width, monitor->logical_height);

    mt_time(NULL);
    ui_init(monitor->logical_width, monitor->logical_height, monitor->scale, mmfm.kuwindow); // DESTROY 3
    mt_time("ui init");

    if (mmfm.record)
    {
	ui_add_cursor();
	evrec_init_recorder(mmfm.rec_path); // DESTROY 4
    }

    if (mmfm.replay)
    {
	ui_add_cursor();
	evrec_init_player(mmfm.rep_path); // DESTROY 5
    }

    ui_update_layout(monitor->logical_width, monitor->logical_height);
    ui_load_folder(config_get("top_path"));
}

/* window update */

void update(ku_event_t ev)
{
    /* printf("UPDATE %i %u %i %i\n", ev.type, ev.time, ev.w, ev.h); */

    if (ev.type == KU_EVENT_RESIZE) ui_update_layout(ev.w, ev.h);
    if (ev.type == KU_EVENT_FRAME) ui_update_player();

    ku_window_event(mmfm.kuwindow, ev);

    if (mmfm.wlwindow->frame_cb == NULL)
    {
	ku_rect_t dirty = ku_window_update(mmfm.kuwindow, 0);

	if (dirty.w > 0 && dirty.h > 0)
	{
	    ku_rect_t sum = ku_rect_add(dirty, mmfm.dirtyrect);

	    /* mt_log_debug("drt %i %i %i %i", (int) dirty.x, (int) dirty.y, (int) dirty.w, (int) dirty.h); */
	    /* mt_log_debug("drt prev %i %i %i %i", (int) mmfm.dirtyrect.x, (int) mmfm.dirtyrect.y, (int) mmfm.dirtyrect.w, (int) mmfm.dirtyrect.h); */
	    /* mt_log_debug("sum aftr %i %i %i %i", (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h); */

	    /* mt_time(NULL); */
	    if (mmfm.softrender) ku_renderer_software_render(mmfm.kuwindow->views, &mmfm.wlwindow->bitmap, sum);
	    else ku_renderer_egl_render(mmfm.kuwindow->views, &mmfm.wlwindow->bitmap, sum);
	    /* mt_time("Render"); */
	    /* nanosleep((const struct timespec[]){{0, 100000000L}}, NULL); */

	    ku_wayland_draw_window(mmfm.wlwindow, (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h);

	    mmfm.dirtyrect = dirty;
	}
    }
}

void update_session(ku_event_t ev)
{
    if (ev.type == KU_EVENT_RESIZE) ui_update_layout(ev.w, ev.h);
    if (ev.type == KU_EVENT_FRAME) ui_update_player();

    ku_window_event(mmfm.kuwindow, ev);
    ku_window_update(mmfm.kuwindow, 0);

    if (mmfm.softrender) ku_renderer_software_render(mmfm.kuwindow->views, &mmfm.wlwindow->bitmap, mmfm.kuwindow->root->frame.local);
    else ku_renderer_egl_render(mmfm.kuwindow->views, &mmfm.wlwindow->bitmap, mmfm.kuwindow->root->frame.local);
}

/* save window buffer to png */

void update_screenshot(uint32_t frame)
{
    static int shotindex = 0;

    char* name = mt_string_new_format(20, "screenshot%.3i.png", shotindex++); // REL 1
    char* path = "";

    if (mmfm.record) path = mt_path_new_append(mmfm.rec_path, name); // REL 2
    if (mmfm.replay) path = mt_path_new_append(mmfm.rep_path, name); // REL 2

    if (mmfm.softrender)
    {
	coder_write_png(path, &mmfm.wlwindow->bitmap);
    }
    else
    {
	ku_bitmap_t* bitmap = ku_bitmap_new(mmfm.wlwindow->width, mmfm.wlwindow->height);
	ku_gl_save_framebuffer(bitmap);
	ku_bitmap_t* flipped = bm_new_flip_y(bitmap); // REL 3
	coder_write_png(path, flipped);
	REL(flipped);
	REL(bitmap);
    }
    ui_update_cursor((ku_rect_t){0, 0, mmfm.wlwindow->width, mmfm.wlwindow->height});

    printf("SCREENHSOT AT %u : %s\n", frame, path);
}

/* window update during recording */

void update_record(ku_event_t ev)
{
    /* normalize floats for deterministic movements during record/replay */
    ev.dx         = floor(ev.dx * 10000) / 10000;
    ev.dy         = floor(ev.dy * 10000) / 10000;
    ev.ratio      = floor(ev.ratio * 10000) / 10000;
    ev.time_frame = floor(ev.time_frame * 10000) / 10000;

    if (ev.type == KU_EVENT_FRAME)
    {
	/* record and send waiting events */
	for (int index = 0; index < mmfm.eventqueue->length; index++)
	{
	    ku_event_t* event = (ku_event_t*) mmfm.eventqueue->data[index];
	    event->frame      = ev.frame;
	    evrec_record(*event);

	    update_session(*event);

	    if (event->type == KU_EVENT_KDOWN && event->keycode == XKB_KEY_Print) update_screenshot(ev.frame);
	    else ui_update_cursor((ku_rect_t){event->x, event->y, 10, 10});
	}

	mt_vector_reset(mmfm.eventqueue);

	/* send frame event */
	update_session(ev);

	/* force frame request if needed */
	if (mmfm.wlwindow->frame_cb == NULL)
	{
	    ku_wayland_draw_window(mmfm.wlwindow, 0, 0, mmfm.wlwindow->width, mmfm.wlwindow->height);
	}
	else mt_log_error("FRAME CALLBACK NOT NULL!!");
    }
    else
    {
	/* queue event */
	void* event = HEAP(ev);
	VADD(mmfm.eventqueue, event);
    }
}

/* window update during replay */

void update_replay(ku_event_t ev)
{
    if (ev.type == KU_EVENT_FRAME)
    {
	// get recorded events
	ku_event_t* recev = NULL;
	while ((recev = evrec_replay(ev.frame)) != NULL)
	{
	    update_session(*recev);

	    if (recev->type == KU_EVENT_KDOWN && recev->keycode == XKB_KEY_Print) update_screenshot(ev.frame);
	    else ui_update_cursor((ku_rect_t){recev->x, recev->y, 10, 10});
	}

	/* send frame event */
	update_session(ev);

	/* force frame request if needed */
	if (mmfm.wlwindow->frame_cb == NULL) ku_wayland_draw_window(mmfm.wlwindow, 0, 0, mmfm.wlwindow->width, mmfm.wlwindow->height);
	else mt_log_error("FRAME CALLBACK NOT NULL!!");
    }
}

void destroy()
{
    ku_wayland_delete_window(mmfm.wlwindow);

    if (mmfm.replay) evrec_destroy();
    if (mmfm.record) evrec_destroy();

    ui_destroy();

    REL(mmfm.kuwindow);

    if (!mmfm.softrender) ku_renderer_egl_destroy();

    SDL_Quit();

    if (mmfm.rec_path) REL(mmfm.rec_path); // REL 14
    if (mmfm.rep_path) REL(mmfm.rep_path); // REL 15
}

int main(int argc, char* argv[])
{
    mt_log_use_colors(isatty(STDERR_FILENO));
    mt_log_level_info();
    mt_time(NULL);

    printf("MultiMedia File Manager v" MMFM_VERSION
	   " by Milan Toth\n"
	   "If you like this app try :\n"
	   "- Visual Music Player (github.com/milgra/vmp)\n"
	   "- Wayland Control Panel ( github.com/milgra/wcp )\n"
	   "- Sway Overview ( github.com/milgra/sov )\n"
	   "- SwayOS (swayos.github.io)\n"
	   "Games :\n"
	   "- Brawl (github.com/milgra/brawl)\n"
	   "- Cortex ( github.com/milgra/cortex )\n"
	   "- Termite (github.com/milgra/termite)\n"
	   "Or donate at www.milgra.com\n\n");

    const char* usage =
	"Usage: sov [options]\n"
	"\n"
	"  -h, --help                          Show help message and quit.\n"
	"  -v                                  Increase verbosity of messages, defaults to errors and warnings only.\n"
	"  -c --config= [config file] \t use config file for session\n"
	"  -d --directory= [config file] \t start with directory\n"
	"  -r --resources= [resources folder] \t use resources dir for session\n"
	"  -s --record= [recorder file] \t record session to file\n"
	"  -p --replay= [recorder file] \t replay session from file\n"
	"  -f --frame= [widthxheight] \t initial window dimension\n"
	"  --software_render \t use software rendering instead of gl accelerated rendering\n"
	"\n";

    const struct option long_options[] =
	{
	    {"help", no_argument, NULL, 'h'},
	    {"verbose", no_argument, NULL, 'v'},
	    {"resources", optional_argument, 0, 'r'},
	    {"directory", optional_argument, 0, 'd'},
	    {"record", optional_argument, 0, 's'},
	    {"software_renderer", optional_argument, 0, 0},
	    {"replay", optional_argument, 0, 'p'},
	    {"config", optional_argument, 0, 'c'},
	    {"frame", optional_argument, 0, 'f'}};

    char* cfg_par = NULL;
    char* res_par = NULL;
    char* rec_par = NULL;
    char* rep_par = NULL;
    char* frm_par = NULL;
    char* dir_par = NULL;

    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "vhr:s:p:c:f:d:", long_options, &option_index)) != -1)
    {
	switch (option)
	{
	    case 0:
		if (option_index == 5) mmfm.softrender = 1;
		/* printf("option %i %s", option_index, long_options[option_index].name); */
		/* if (optarg) printf(" with arg %s", optarg); */
		break;
	    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
	    case 'c': cfg_par = mt_string_new_cstring(optarg); break; // REL 0
	    case 'r': res_par = mt_string_new_cstring(optarg); break; // REL 1
	    case 's': rec_par = mt_string_new_cstring(optarg); break; // REL 2
	    case 'p': rep_par = mt_string_new_cstring(optarg); break; // REL 3
	    case 'f': frm_par = mt_string_new_cstring(optarg); break; // REL 4
	    case 'd': dir_par = mt_string_new_cstring(optarg); break; // REL 4
	    case 'v': mt_log_inc_verbosity(); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    if (rec_par) mmfm.record = 1;
    if (rep_par) mmfm.replay = 1;

    srand((unsigned int) time(NULL));

    char cwd[PATH_MAX] = {"~"};
    getcwd(cwd, sizeof(cwd));

    char* wrk_path    = mt_path_new_normalize(cwd, NULL);                                                                             // REL 5
    char* res_path    = res_par ? mt_path_new_normalize(res_par, wrk_path) : mt_string_new_cstring(PKG_DATADIR);                      // REL 7
    char* cfgdir_path = cfg_par ? mt_path_new_normalize(cfg_par, wrk_path) : mt_path_new_normalize("~/.config/mmfm", getenv("HOME")); // REL 8
    char* css_path    = mt_path_new_append(res_path, "html/main.css");                                                                // REL 9
    char* html_path   = mt_path_new_append(res_path, "html/main.html");                                                               // REL 10
    char* cfg_path    = mt_path_new_append(cfgdir_path, "config.kvl");                                                                // REL 12
    char* per_path    = mt_path_new_append(cfgdir_path, "state.kvl");                                                                 // REL 13
    char* rec_path    = rec_par ? mt_path_new_normalize(rec_par, wrk_path) : NULL;                                                    // REL 14
    char* rep_path    = rep_par ? mt_path_new_normalize(rep_par, wrk_path) : NULL;                                                    // REL 15
    char* dir_path    = dir_par ? mt_path_new_normalize(dir_par, wrk_path) : NULL;                                                    // REL 15

    // print path info to console

    mt_log_debug("working path  : %s", wrk_path);
    mt_log_debug("resource path : %s", res_path);
    mt_log_debug("config path   : %s", cfg_path);
    mt_log_debug("directory path   : %s", dir_path);
    mt_log_debug("state path   : %s", per_path);
    mt_log_debug("css path      : %s", css_path);
    mt_log_debug("html path     : %s", html_path);
    mt_log_debug("record path   : %s", rec_path);
    mt_log_debug("replay path   : %s", rep_path);

    // init config

    config_init(); // DESTROY 0

    // set defaults before overwriting those with saved vales

    config_set("dark_mode", "false");
    config_set("res_path", res_path);
    config_set_bool("sidebar_visible", 1);

    // read config, it overwrites defaults if exists

    config_read(cfg_path);

    // init non-configurable defaults

    config_set("top_path", dir_path ? dir_path : wrk_path);
    config_set("wrk_path", wrk_path);
    config_set("cfg_path", cfg_path);
    config_set("per_path", per_path);
    config_set("css_path", css_path);
    config_set("html_path", html_path);

    /* this two shouldn't go into the config file because of record/replay */

    mmfm.rec_path = rec_path;
    mmfm.rep_path = rep_path;

    if (rec_path) evrec_init_recorder(rec_path); // DESTROY 4
    if (rep_path) evrec_init_player(rep_path);

    if (rec_path != NULL) ku_wayland_init(init, update_record, destroy, 0);
    else if (rep_path != NULL) ku_wayland_init(init, update_replay, destroy, 16);
    else ku_wayland_init(init, update, destroy, 0);

    config_destroy(); // DESTROY 0

    // cleanup

    if (cfg_par) REL(cfg_par); // REL 0
    if (res_par) REL(res_par); // REL 1
    if (rec_par) REL(rec_par); // REL 2
    if (rep_par) REL(rep_par); // REL 3
    if (frm_par) REL(frm_par); // REL 4

    REL(wrk_path);    // REL 6
    REL(res_path);    // REL 7
    REL(cfgdir_path); // REL 8
    REL(css_path);    // REL 9
    REL(html_path);   // REL 10
    REL(cfg_path);    // REL 12
    REL(per_path);    // REL 13

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif

    return 0;
}

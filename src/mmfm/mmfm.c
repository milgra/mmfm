#include "coder.c"
#include "config.c"
#include "filemanager.c"
#include "ku_connector_wayland.c"
#include "ku_gl.c"
#include "ku_png.c"
#include "ku_recorder.c"
#include "ku_renderer_egl.c"
#include "ku_renderer_soft.c"
#include "ku_window.c"
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
    wl_window_t* wlwindow;
    ku_window_t* kuwindow;

    ku_rect_t dirtyrect;
    int       softrender;

    int   autotest;
    char* pngpath;

    int width;
    int height;
} mmfm = {0};

void init(wl_event_t event)
{
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_AUDIO);

    if (mmfm.softrender)
    {
	mmfm.wlwindow = ku_wayland_create_window("mmfm", mmfm.width, mmfm.height);
	ku_wayland_show_window(mmfm.wlwindow);
    }
    else
    {
	mmfm.wlwindow = ku_wayland_create_eglwindow("mmfm", mmfm.width, mmfm.height);
	ku_wayland_show_window(mmfm.wlwindow);

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
}

void load(wl_window_t* info)
{
    mmfm.kuwindow = ku_window_create(info->buffer_width, info->buffer_height, info->scale);

    ui_init(info->buffer_width, info->buffer_height, info->scale, mmfm.kuwindow); // DESTROY 3

    if (mmfm.autotest) ui_add_cursor();

    ui_update_layout(info->buffer_width, info->buffer_height);

    ku_window_layout(mmfm.kuwindow);
}

void load_data()
{
    ui_load_folder(config_get("top_path"));
    ui_load_file(config_get("opn_path"));
}

/* window update */

void update(ku_event_t ev)
{
    /* printf("UPDATE %i %u %i %i\n", ev.type, ev.time, ev.x, ev.y); */

    if (ev.type == KU_EVENT_WINDOW_SHOWN) load(ev.window);        /* set ui size on window enter, load files */
    if (ev.type == KU_EVENT_RESIZE) ui_update_layout(ev.w, ev.h); /* change layout if needed */
    if (ev.type == KU_EVENT_FRAME) ui_update_player();            /* update player state if needed */
    if (!mmfm.kuwindow) return;                                   /* TODO avoid this more gracefully */

    ku_window_event(mmfm.kuwindow, ev); /* regular events */

    if (mmfm.autotest)
    {
	/* create screenshot if needed */
	if (ev.type == KU_EVENT_KEY_DOWN && ev.keycode == XKB_KEY_Print)
	{
	    static int shotindex = 0;

	    char* name = mt_string_new_format(20, "screenshot%.3i.png", shotindex++);
	    char* path = mt_path_new_append(mmfm.pngpath, name);

	    if (mmfm.softrender) ku_renderer_soft_screenshot(&mmfm.wlwindow->bitmap, path);
	    else ku_renderer_egl_screenshot(&mmfm.wlwindow->bitmap, path);

	    ui_update_cursor((ku_rect_t){0, 0, mmfm.wlwindow->width, mmfm.wlwindow->height});

	    printf("SCREENHSOT AT %u : %s\n", ev.frame, path);
	}
	else ui_update_cursor((ku_rect_t){ev.x, ev.y, 10, 10}); /* update virtual cursor if needed */
    }

    if (mmfm.wlwindow->frame_cb == NULL)
    {
	ku_rect_t dirty = ku_window_update(mmfm.kuwindow, 0);

	/* in case of record/replay force continuous draw by full size dirty rect */
	if (mmfm.autotest) dirty = mmfm.kuwindow->root->frame.local;

	if (dirty.w > 0 && dirty.h > 0)
	{
	    /* add previous dirty rect to current dirty rect to redraw disappearing parts */
	    ku_rect_t sum = ku_rect_add(dirty, mmfm.dirtyrect);

	    if (mmfm.softrender) ku_renderer_software_render(mmfm.kuwindow->views, &mmfm.wlwindow->bitmap, sum);
	    else ku_renderer_egl_render(mmfm.kuwindow->views, &mmfm.wlwindow->bitmap, sum);

	    /* frame done request must be requested before draw */
	    ku_wayland_request_frame(mmfm.wlwindow);
	    ku_wayland_draw_window(mmfm.wlwindow, (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h);

	    /* store current dirty rect for next draw */
	    mmfm.dirtyrect = dirty;
	}
    }

    if (ev.type == KU_EVENT_WINDOW_SHOWN) load_data(); /* set ui size on window enter, load files */
}

void destroy()
{
    ui_destroy();
    REL(mmfm.kuwindow);
    ku_wayland_delete_window(mmfm.wlwindow);

    if (!mmfm.softrender) ku_renderer_egl_destroy();

    SDL_Quit();
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
	"Usage: sov [file] [options]\n"
	"\n"
	"  -h, --help                          Show help message and quit.\n"
	"  -v                                  Increase verbosity of messages, defaults to errors and warnings only.\n"
	"  -c --config= [config file]          Use config file for session\n"
	"  -d --directory= [config file]       Start with directory\n"
	"  -r --resources= [resources folder]  Use resources dir for session\n"
	"  -s --record= [recorder file]        Record session to file\n"
	"  -p --replay= [recorder file]        Replay session from file\n"
	"  -f --frame= [widthxheight]          Initial window dimension\n"
	"  --software_render                   Use software rendering instead of gl accelerated rendering\n"
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
	    {"frame", optional_argument, 0, 'f'},
	    {NULL, 0, NULL, 0}};

    char* cfg_par = NULL;
    char* res_par = NULL;
    char* rec_par = NULL;
    char* rep_par = NULL;
    char* frm_par = NULL;
    char* dir_par = NULL;
    char* opn_par = NULL;

    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, ":vhr:s:p:c:f:d:", long_options, &option_index)) != -1)
    {
	switch (option)
	{
	    case 0:
		if (option_index == 5) mmfm.softrender = 1;
		printf("option %i %s\n", option_index, long_options[option_index].name);
		if (optarg) printf(" with arg %s\n", optarg);
		break;
	    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
	    case 'c': cfg_par = mt_string_new_cstring(optarg); break; // REL 0
	    case 'r': res_par = mt_string_new_cstring(optarg); break; // REL 1
	    case 's': rec_par = mt_string_new_cstring(optarg); break; // REL 2
	    case 'p': rep_par = mt_string_new_cstring(optarg); break; // REL 3
	    case 'f': frm_par = mt_string_new_cstring(optarg); break; // REL 4
	    case 'd': dir_par = mt_string_new_cstring(optarg); break; // REL 4
	    case 'v': mt_log_inc_verbosity(); break;
	    case ':': printf("MISSING\n"); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    if (optind < argc) opn_par = mt_string_new_cstring(argv[optind]);

    srand((unsigned int) time(NULL));

    char  cwd[PATH_MAX] = {"~"};
    char* res           = getcwd(cwd, sizeof(cwd));

    if (res == NULL) mt_log_error("getcwd error");

    char* wrk_path    = mt_path_new_normalize(cwd, NULL);                                                                             // REL 5
    char* res_path    = res_par ? mt_path_new_normalize(res_par, wrk_path) : mt_string_new_cstring(PKG_DATADIR);                      // REL 7
    char* cfgdir_path = cfg_par ? mt_path_new_normalize(cfg_par, wrk_path) : mt_path_new_normalize("~/.config/mmfm", getenv("HOME")); // REL 8
    char* img_path    = mt_path_new_append(res_path, "img");                                                                          // REL 9
    char* css_path    = mt_path_new_append(res_path, "html/main.css");                                                                // REL 9
    char* html_path   = mt_path_new_append(res_path, "html/main.html");                                                               // REL 10
    char* cfg_path    = mt_path_new_append(cfgdir_path, "config.kvl");                                                                // REL 12
    char* per_path    = mt_path_new_append(cfgdir_path, "state.kvl");                                                                 // REL 13
    char* rec_path    = rec_par ? mt_path_new_normalize(rec_par, wrk_path) : NULL;                                                    // REL 14
    char* rep_path    = rep_par ? mt_path_new_normalize(rep_par, wrk_path) : NULL;                                                    // REL 15
    char* dir_path    = dir_par ? mt_path_new_normalize(dir_par, wrk_path) : NULL;                                                    // REL 15

    // print path info to console

    mt_log_debug("working path   : %s", wrk_path);
    mt_log_debug("resource path  : %s", res_path);
    mt_log_debug("config path    : %s", cfg_path);
    mt_log_debug("directory path : %s", dir_path);
    mt_log_debug("open file path : %s", opn_par);
    mt_log_debug("state path     : %s", per_path);
    mt_log_debug("img path       : %s", img_path);
    mt_log_debug("css path       : %s", css_path);
    mt_log_debug("html path      : %s", html_path);
    mt_log_debug("record path    : %s", rec_path);
    mt_log_debug("replay path    : %s", rep_path);

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
    config_set("img_path", img_path);
    config_set("wrk_path", wrk_path);
    config_set("opn_path", opn_par);
    config_set("cfg_path", cfg_path);
    config_set("per_path", per_path);
    config_set("css_path", css_path);
    config_set("html_path", html_path);

    if (frm_par != NULL)
    {
	mmfm.width  = atoi(frm_par);
	char* next  = strstr(frm_par, "x");
	mmfm.height = atoi(next + 1);
    }
    else
    {
	/* TODO calc this based on output size */
	mmfm.width  = 1200;
	mmfm.height = 600;
    }

    if (rec_path || rep_path)
    {
	ku_recorder_init(update);
	mmfm.autotest = 1;
	config_set("autotest", "autotest");
    }
    else config_set("autotest", NULL); /* TODO saved temporary values fuck up things, change this */

    if (rec_path)
    {
	char* tgt_path = mt_path_new_append(rec_path, "session.rec");
	ku_recorder_record(tgt_path);
	REL(tgt_path);
	mmfm.pngpath = rec_path;
    }
    if (rep_path)
    {
	char* tgt_path = mt_path_new_append(rep_path, "session.rec");
	ku_recorder_replay(tgt_path);
	REL(tgt_path);
	mmfm.pngpath = rep_path;
    }

    /* proxy events through the recorder in case of record/replay */
    if (rec_path || rep_path) ku_wayland_init(init, ku_recorder_update, destroy, 0);
    else ku_wayland_init(init, update, destroy, 0);

    config_destroy(); // DESTROY 0

    // cleanup

    ku_recorder_destroy();

    if (opn_par) REL(opn_par);
    if (cfg_par) REL(cfg_par);
    if (res_par) REL(res_par);
    if (rec_par) REL(rec_par);
    if (rep_par) REL(rep_par);
    if (frm_par) REL(frm_par);
    if (dir_par) REL(dir_par);

    if (dir_path) REL(dir_path);
    REL(img_path);
    REL(wrk_path);
    REL(res_path);
    REL(cfgdir_path);
    REL(css_path);
    REL(html_path);
    REL(cfg_path);
    REL(per_path);

#ifdef MT_MEMORY_DEBUG
    mt_memory_stats();
#endif

    return 0;
}

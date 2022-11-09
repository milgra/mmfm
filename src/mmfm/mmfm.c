#include "coder.c"
#include "config.c"
#include "evrecorder.c"
#include "filemanager.c"
#include "ku_connector_wayland.c"
#include "ku_gl.c"
#include "ku_renderer_egl.c"
#include "ku_renderer_soft.c"
#include "ku_window.c"
#include "ui.c"
#include "zc_bitmap_ext.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_map.c"
#include "zc_path.c"
#include "zc_time.c"
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
    struct wl_window* window;
    ku_rect_t         dirty_prev;
    ku_window_t*      kuwindow;

    int first_render;
    int use_software_renderer;
} mmfm = {0};

void init(wl_event_t event)
{
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_Init(SDL_INIT_AUDIO);

    struct monitor_info* monitor = event.monitors[0];

    if (mmfm.use_software_renderer)
    {
	mmfm.window = ku_wayland_create_window("MMFM", 1200, 600);
    }
    else
    {
	mmfm.window = ku_wayland_create_eglwindow("MMFM", 1200, 600);

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

    zc_time(NULL);
    ui_init(monitor->logical_width, monitor->logical_height, monitor->scale, mmfm.kuwindow); // DESTROY 3
    zc_time("ui init");

    if (mmfm.record)
    {
	evrec_init_recorder(config_get("rec_path")); // DESTROY 4
    }

    if (mmfm.replay)
    {
	evrec_init_player(config_get("rep_path")); // DESTROY 5
	ui_add_cursor();
    }

    ui_update_layout(monitor->logical_width, monitor->logical_height);
}

void event(wl_event_t event)
{
}

void update(ku_event_t ev)
{
    /* printf("UPDATE %i %i %i\n", ev.type, ev.w, ev.h); */

    /* if (ev.type == KU_EVENT_RESIZE) ku_view_describe(mmfm.kuwindow->root, 0); */
    if (ev.type == KU_EVENT_RESIZE) ui_update_layout(ev.w, ev.h);
    if (ev.type == KU_EVENT_FRAME) ui_update_player();

    ku_window_event(mmfm.kuwindow, ev);

    if (mmfm.window->frame_cb == NULL)
    {
	ku_rect_t dirty = ku_window_update(mmfm.kuwindow, 0);

	if (dirty.w > 0 && dirty.h > 0)
	{
	    ku_rect_t sum = ku_rect_add(dirty, mmfm.dirty_prev);

	    /* zc_log_debug("drt %i %i %i %i", (int) dirty.x, (int) dirty.y, (int) dirty.w, (int) dirty.h); */
	    /* zc_log_debug("drt prev %i %i %i %i", (int) mmfm.dirty_prev.x, (int) mmfm.dirty_prev.y, (int) mmfm.dirty_prev.w, (int) mmfm.dirty_prev.h); */
	    /* zc_log_debug("sum aftr %i %i %i %i", (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h); */

	    /* zc_time(NULL); */
	    if (mmfm.use_software_renderer) ku_renderer_software_render(mmfm.kuwindow->views, &mmfm.window->bitmap, sum);
	    else ku_renderer_egl_render(mmfm.kuwindow->views, &mmfm.window->bitmap, sum);
	    /* zc_time("Render"); */
	    /* nanosleep((const struct timespec[]){{0, 100000000L}}, NULL); */

	    ku_wayland_draw_window(mmfm.window, (int) sum.x, (int) sum.y, (int) sum.w, (int) sum.h);

	    mmfm.dirty_prev = dirty;
	}
    }

    // post-first-render init

    if (mmfm.first_render == 0)
    {
	mmfm.first_render = 1;
	ui_load_folder(config_get("top_path"));
    }
}

void update_record(ku_event_t ev)
{
    update(ev);

    evrec_record(ev);

    if (ev.type == KU_EVENT_KDOWN && ev.keycode == XKB_KEY_Print)
    {
	static int shotindex = 0;

	char* name = cstr_new_format(20, "screenshot%.3i.png", shotindex++); // REL 1
	char* path = path_new_append(config_get("rec_path"), name);          // REL 2

	if (mmfm.use_software_renderer)
	{
	    coder_write_png(path, &mmfm.window->bitmap);
	}
	else
	{
	    ku_bitmap_t* bitmap = ku_bitmap_new(mmfm.window->width, mmfm.window->height);
	    ku_gl_save_framebuffer(bitmap);
	    ku_bitmap_t* flipped = bm_new_flip_y(bitmap); // REL 3
	    coder_write_png(path, flipped);
	    REL(flipped);
	    REL(bitmap);
	}
    }
}

void update_replay(ku_event_t ev)
{
    if (ev.type == KU_EVENT_TIME)
    {
	// get recorded events
	ku_event_t* recev = NULL;
	while ((recev = evrec_replay(ev.time)) != NULL)
	{
	    update(*recev);
	}
    }

    update(ev);
}

void render(uint32_t time, uint32_t index, ku_bitmap_t* bm)
{
    /* printf("RENDER\n"); */
    /* zc_time(NULL); */
    /* int changed = ku_window_render(mmfm.kuwindow,time, bm); */
    /* zc_time("render"); */
}

void destroy()
{
    if (mmfm.replay) evrec_destroy(); // DESTROY 5
    if (mmfm.record) evrec_destroy(); // DESTROY 4

    ui_destroy(); // DESTROY 3

    REL(mmfm.kuwindow);

    SDL_Quit(); // QUIT 0
}

int main(int argc, char* argv[])
{
    zc_log_use_colors(isatty(STDERR_FILENO));
    zc_log_level_info();
    zc_time(NULL);

    printf("MultiMedia File Manager v" MMFM_VERSION
	   " by Milan Toth ( www.milgra.com )\n"
	   "If you like this app try :\n"
	   "- Visual Music Player (github.com/milgra/vmp)\n"
	   "- Wayland Control Panel ( github.com/milgra/wcp )\n"
	   "- Sway Overview ( github.com/milgra/sov )\n"
	   "- SwayOS (swayos.github.io)\n"
	   "Games :\n"
	   "- Brawl (github.com/milgra/brawl)\n"
	   "- Cortex ( github.com/milgra/cortex )\n"
	   "- Termite (github.com/milgra/termite)\n\n");

    const char* usage =
	"Usage: sov [options]\n"
	"\n"
	"  -h, --help                          Show help message and quit.\n"
	"  -v                                  Increase verbosity of messages, defaults to errors and warnings only.\n"
	"  -c --config= [config file] \t use config file for session\n"
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

    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "vhr:s:p:c:f:", long_options, &option_index)) != -1)
    {
	switch (option)
	{
	    case 0:
		if (option_index == 4) mmfm.use_software_renderer = 1;
		/* printf("option %i %s", option_index, long_options[option_index].name); */
		/* if (optarg) printf(" with arg %s", optarg); */
		break;
	    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
	    case 'c': cfg_par = cstr_new_cstring(optarg); break; // REL 0
	    case 'r': res_par = cstr_new_cstring(optarg); break; // REL 1
	    case 's': rec_par = cstr_new_cstring(optarg); break; // REL 2
	    case 'p': rep_par = cstr_new_cstring(optarg); break; // REL 3
	    case 'f': frm_par = cstr_new_cstring(optarg); break; // REL 4
	    case 'v': zc_log_inc_verbosity(); break;
	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    if (rec_par) mmfm.record = 1;
    if (rep_par) mmfm.replay = 1;

    srand((unsigned int) time(NULL));

    char cwd[PATH_MAX] = {"~"};
    getcwd(cwd, sizeof(cwd));

    char* wrk_path    = path_new_normalize(cwd, NULL);                                                                          // REL 5
    char* res_path    = res_par ? path_new_normalize(res_par, wrk_path) : cstr_new_cstring(PKG_DATADIR);                        // REL 7
    char* cfgdir_path = cfg_par ? path_new_normalize(cfg_par, wrk_path) : path_new_normalize("~/.config/mmfm", getenv("HOME")); // REL 8
    char* css_path    = path_new_append(res_path, "html/main.css");                                                             // REL 9
    char* html_path   = path_new_append(res_path, "html/main.html");                                                            // REL 10
    char* cfg_path    = path_new_append(cfgdir_path, "config.kvl");                                                             // REL 12
    char* per_path    = path_new_append(cfgdir_path, "state.kvl");                                                              // REL 13
    char* rec_path    = rec_par ? path_new_normalize(rec_par, wrk_path) : NULL;                                                 // REL 14
    char* rep_path    = rep_par ? path_new_normalize(rep_par, wrk_path) : NULL;                                                 // REL 15

    // print path info to console

    zc_log_debug("working path  : %s", wrk_path);
    zc_log_debug("resource path : %s", res_path);
    zc_log_debug("config path   : %s", cfg_path);
    zc_log_debug("state path   : %s", per_path);
    zc_log_debug("css path      : %s", css_path);
    zc_log_debug("html path     : %s", html_path);
    zc_log_debug("record path   : %s", rec_path);
    zc_log_debug("replay path   : %s", rep_path);

    // init config

    config_init(); // DESTROY 0

    // set defaults before overwriting those with saved vales

    config_set("dark_mode", "false");
    config_set("res_path", res_path);
    config_set_bool("sidebar_visible", 1);

    // read config, it overwrites defaults if exists

    config_read(cfg_path);

    // init non-configurable defaults

    config_set("wrk_path", wrk_path);
    config_set("cfg_path", cfg_path);
    config_set("per_path", per_path);
    config_set("css_path", css_path);
    config_set("html_path", html_path);

    if (rec_path) config_set("rec_path", rec_path);
    if (rep_path) config_set("rep_path", rep_path);

    if (rec_path) evrec_init_recorder(rec_path); // DESTROY 4
    if (rep_path) evrec_init_player(rep_path);

    if (rec_path != NULL) ku_wayland_init(init, event, update_record, render, destroy, 0);
    else if (rep_path != NULL) ku_wayland_init(init, event, update_replay, render, destroy, 16);
    else ku_wayland_init(init, event, update, render, destroy, 0);

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

    if (rec_path) REL(rec_path); // REL 14
    if (rep_path) REL(rep_path); // REL 15

#ifdef DEBUG
    mem_stats();
#endif

    return 0;
}

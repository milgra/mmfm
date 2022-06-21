#include "callbacks.c"
#include "config.c"
#include "evrecorder.c"
#include "filemanager.c"
#include "player.c"
#include "ui.c"
#include "ui_compositor.c"
#include "ui_manager.c"
#include "ui_visualizer.c"
#include "visible.c"
#include "wm_connector.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_map.c"
#include "zc_path.c"
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

struct
{
    char replay;
    char record;
} mmfm = {0};

void init(int width, int height)
{
    player_init();          // DESTROY 0
    visible_init();         // DESTROY 1
    callbacks_init();       // DESTROY 2
    ui_init(width, height); // DESTROY 3

    if (mmfm.record)
    {
	evrec_init_recorder(config_get("rec_path")); // DESTROY 4
    }

    if (mmfm.replay)
    {
	evrec_init_player(config_get("rep_path")); // DESTROY 5
	ui_add_cursor();
    }

    // load current directory

    map_t* files = MNEW(); // REL 0
    fm_list(config_get("top_path"), files);

    visible_set_files(files);
    visible_set_sortfield("basename", 0);

    REL(files); // REL 0
}

void update(ev_t ev)
{
    if (ev.type == EV_TIME)
    {

	if (mmfm.replay)
	{
	    // get recorded events
	    ev_t* recev = NULL;
	    while ((recev = evrec_replay(ev.time)) != NULL)
	    {
		ui_manager_event(*recev);
		ui_update_cursor((r2_t){recev->x, recev->y, 10, 10});

		if (recev->type == EV_KDOWN && recev->keycode == SDLK_PRINTSCREEN) ui_save_screenshot(ev.time, mmfm.replay);
	    }
	}
    }
    else
    {
	if (mmfm.record)
	{
	    evrec_record(ev);
	    if (ev.type == EV_KDOWN && ev.keycode == SDLK_PRINTSCREEN) ui_save_screenshot(ev.time, mmfm.replay);
	}
    }

    // in case of replay only send time events
    if (!mmfm.replay || ev.type == EV_TIME) ui_manager_event(ev);
}

void render(uint32_t time)
{
    ui_manager_render(time);
}

void destroy()
{
    if (mmfm.replay) evrec_destroy(); // DESTROY 5
    if (mmfm.record) evrec_destroy(); // DESTROY 4

    ui_destroy();        // DESTROY 3
    callbacks_destroy(); // DESTROY 2
    visible_destroy();   // DESTROY 1
    player_destroy();    // DESTROY 0
}

int main(int argc, char* argv[])
{
    zc_log_use_colors(isatty(STDERR_FILENO));
    zc_log_level_info();

    printf("MultiMedia File Manager v" MMFM_VERSION " by Milan Toth ( www.milgra.com )\n");

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
	"\n";

    const struct option long_options[] =
	{
	    {"help", no_argument, NULL, 'h'},
	    {"verbose", no_argument, NULL, 'v'},
	    {"resources", optional_argument, 0, 'r'},
	    {"record", optional_argument, 0, 's'},
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

    char* top_path    = path_new_normalize(cwd, NULL);                                                                          // REL 5
    char* wrk_path    = path_new_normalize(SDL_GetBasePath(), NULL);                                                            // REL 6
    char* res_path    = res_par ? path_new_normalize(res_par, wrk_path) : cstr_new_cstring("/usr/share/mmfm");                  // REL 7
    char* cfgdir_path = cfg_par ? path_new_normalize(cfg_par, wrk_path) : path_new_normalize("~/.config/mmfm", getenv("HOME")); // REL 8
    char* css_path    = path_new_append(res_path, "html/main.css");                                                             // REL 9
    char* html_path   = path_new_append(res_path, "html/main.html");                                                            // REL 10
    char* font_path   = path_new_append(res_path, "Baloo.ttf");                                                                 // REL 11
    char* cfg_path    = path_new_append(cfgdir_path, "config.kvl");                                                             // REL 12
    char* rec_path    = rec_par ? path_new_normalize(rec_par, wrk_path) : NULL;                                                 // REL 13
    char* rep_path    = rep_par ? path_new_normalize(rep_par, wrk_path) : NULL;                                                 // REL 14

    // print path info to console

    zc_log_debug("top path      : %s", top_path);
    zc_log_debug("working path  : %s", wrk_path);
    zc_log_debug("resource path : %s", res_path);
    zc_log_debug("config path   : %s", cfg_path);
    zc_log_debug("css path      : %s", css_path);
    zc_log_debug("html path     : %s", html_path);
    zc_log_debug("font path     : %s", font_path);
    zc_log_debug("record path   : %s", rec_path);
    zc_log_debug("replay path   : %s", rep_path);

    // init config

    config_init(); // DESTROY 0

    config_set("dark_mode", "false");
    config_set("res_path", res_path);

    // read config, it overwrites defaults if exists

    config_read(cfg_path);

    // init non-configurable defaults

    config_set("top_path", top_path);
    config_set("wrk_path", wrk_path);
    config_set("cfg_path", cfg_path);
    config_set("css_path", css_path);
    config_set("html_path", html_path);
    config_set("font_path", font_path);

    if (rec_path) config_set("rec_path", rec_path);
    if (rep_path) config_set("rep_path", rep_path);

    // cleanup

    if (cfg_par) REL(cfg_par); // REL 0
    if (res_par) REL(res_par); // REL 1
    if (rec_par) REL(rec_par); // REL 2
    if (rec_par) REL(rep_par); // REL 3
    if (frm_par) REL(frm_par); // REL 4

    REL(top_path);    // REL 5
    REL(wrk_path);    // REL 6
    REL(res_path);    // REL 7
    REL(cfgdir_path); // REL 8
    REL(css_path);    // REL 9
    REL(html_path);   // REL 10
    REL(font_path);   // REL 11
    REL(cfg_path);    // REL 12

    if (rec_path) REL(rec_path); // REL 13
    if (rep_path) REL(rep_path); // REL 14

    wm_loop(init, update, render, destroy, frm_par);

    config_destroy(); // DESTROY 0

#ifdef DEBUG
    mem_stats();
#endif

    return 0;
}

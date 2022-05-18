#include "bm_rgba_util.c"
#include "callbacks.c"
#include "coder.c"
#include "config.c"
#include "evrecorder.c"
#include "files.c"
#include "library.c"
#include "pdf.c"
#include "player.c"
#include "ui.c"
#include "ui_compositor.c"
#include "ui_filelist.c"
#include "ui_manager.c"
#include "ui_play_controls.c"
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

void load_directory()
{
  assert(config_get("top_path") != NULL);

  //  db_reset();
  // db_read(config_get("lib_path"));

  map_t* files = MNEW();                         // REL 0
  lib_read_files(config_get("top_path"), files); // read all files under library path

  /* printf("FILES\n"); */
  /* mem_describe(files, 0); */

  visible_set_files(files);

  visible_set_sortfield("basename", 0);

  ui_filelist_refresh();

  /* db_update(files);                              // remove deleted files from db, remove existing files from files */

  /* if (files->count > 0) */
  /* { */
  /*   LOG("new files detected : %i", files->count); */
  /*   lib_analyze_files(mmfm.lib_ch, files); // start analyzing new entries */
  /* } */

  // visible_set_sortfield("meta/artist", 0);

  REL(files); // REL 0
}

void save_screenshot(uint32_t time)
{
  if (config_get("lib_path"))
  {
    static int cnt    = 0;
    view_t*    root   = ui_manager_get_root();
    r2_t       frame  = root->frame.local;
    bm_rgba_t* screen = bm_rgba_new(frame.w, frame.h); // REL 0

    // remove cursor for screenshot to remain identical

    if (mmfm.replay) ui_render_without_cursor(time);

    ui_compositor_render_to_bmp(screen);

    char*      name    = cstr_new_format(20, "screenshot%.3i.png", cnt++); // REL 1
    char*      path    = path_new_append(config_get("lib_path"), name);    // REL 2
    bm_rgba_t* flipped = bm_rgba_new_flip_y(screen);                       // REL 3

    coder_write_png(path, flipped);

    REL(flipped); // REL 3
    REL(name);    // REL 2
    REL(path);    // REL 1
    REL(screen);  // REL 0

    if (mmfm.replay) ui_update_cursor(frame); // full screen cursor to indicate screenshot, next step will reset it
  }
}

void init(int width, int height)
{
  player_init();          // destroy 0
  visible_init();         // destroy 1
  callbacks_init();       // destroy 2
  ui_init(width, height); // destroy 3

  if (mmfm.record)
  {
    evrec_init_recorder(config_get("rec_path")); // destroy 4
  }

  if (mmfm.replay)
  {
    evrec_init_player(config_get("rep_path")); // destroy 5
    ui_add_cursor();
  }

  load_directory();
}

void update(ev_t ev)
{
  if (ev.type == EV_TIME)
  {
    ui_play_update();
    // if (!mmfm.pdfbmp) mmfm.pdfbmp = pdf_render("/home/milgra/Projects/mmfm/ajanlat.pdf");
    // ui_visualizer_show_image(mmfm.pdfbmp); // show pdf

    if (mmfm.replay)
    {
      // get recorded events
      ev_t* recev = NULL;
      while ((recev = evrec_replay(ev.time)) != NULL)
      {
        ui_manager_event(*recev);
        ui_update_cursor((r2_t){recev->x, recev->y, 10, 10});

        if (recev->type == EV_KDOWN && recev->keycode == SDLK_PRINTSCREEN) save_screenshot(ev.time);
      }
    }
  }
  else
  {
    if (mmfm.record)
    {
      evrec_record(ev);
      if (ev.type == EV_KDOWN && ev.keycode == SDLK_PRINTSCREEN) save_screenshot(ev.time);
    }
  }

  // in case of replay only send time events
  if (!mmfm.replay || ev.type == EV_TIME) ui_manager_event(ev);
}

// render, called once per frame

void render(uint32_t time)
{
  ui_manager_render(time);
}

void destroy()
{
  if (mmfm.replay) evrec_destroy(); // destroy 5
  if (mmfm.record) evrec_destroy(); // destroy 4

  ui_destroy();        // destroy 3
  callbacks_destroy(); // destroy 2
  visible_destroy();   // destroy 1
  player_destroy();    // destroy 0

#ifdef DEBUG
  mem_stats();
#endif
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

  srand((unsigned int)time(NULL));

  char cwd[PATH_MAX] = {"~"};
  getcwd(cwd, sizeof(cwd));

  char* top_path    = path_new_normalize(cwd, NULL);
  char* wrk_path    = path_new_normalize(SDL_GetBasePath(), NULL);                                                            // REL 0
  char* res_path    = res_par ? path_new_normalize(res_par, wrk_path) : cstr_new_cstring("/usr/share/mmfm");                  // REL 1
  char* cfgdir_path = cfg_par ? path_new_normalize(cfg_par, wrk_path) : path_new_normalize("~/.config/mmfm", getenv("HOME")); // REL 2
  char* css_path    = path_new_append(res_path, "main.css");                                                                  // REL 3
  char* html_path   = path_new_append(res_path, "main.html");                                                                 // REL 4
  char* font_path   = path_new_append(res_path, "Baloo.ttf");                                                                 // REL 5
  char* cfg_path    = path_new_append(cfgdir_path, "config.kvl");                                                             // REL 6
  char* rec_path    = rec_par ? path_new_normalize(rec_par, wrk_path) : NULL;                                                 // REL 7
  char* rep_path    = rep_par ? path_new_normalize(rep_par, wrk_path) : NULL;                                                 // REL 8

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

  REL(wrk_path);
  REL(res_path);
  REL(cfgdir_path);
  REL(css_path);
  REL(html_path);
  REL(font_path);
  REL(cfg_path);

  if (rec_path) REL(rec_path);
  if (rep_path) REL(rep_path);

  wm_loop(init, update, render, destroy, frm_par);

  config_destroy(); // DESTROY 0

  return 0;
}

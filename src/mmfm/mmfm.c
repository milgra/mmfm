#include "bm_rgba_util.c"
#include "callbacks.c"
#include "coder.c"
#include "config.c"
#include "evrecorder.c"
#include "files.c"
#include "library.c"
#include "pdf.c"
#include "player.c"
#include "tg_css.c"
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
  char* cfg_par; // config path parameter
  char* res_par; // resources path parameter

  char* rec_par; // record parameter
  char* rep_par; // replay parameter
  char* frm_par; // frame parameter

} zm = {0};

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
  /*   lib_analyze_files(zm.lib_ch, files); // start analyzing new entries */
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

    if (zm.rep_par) ui_render_without_cursor(time);

    ui_compositor_render_to_bmp(screen);

    char*      name    = cstr_new_format(20, "screenshot%.3i.png", cnt++); // REL 1
    char*      path    = path_new_append(config_get("lib_path"), name);    // REL 2
    bm_rgba_t* flipped = bm_rgba_new_flip_y(screen);                       // REL 3

    coder_write_png(path, flipped);

    REL(flipped); // REL 3
    REL(name);    // REL 2
    REL(path);    // REL 1
    REL(screen);  // REL 0

    if (zm.rep_par) ui_update_cursor(frame); // full screen cursor to indicate screenshot, next step will reset it
  }
}

void init(int width, int height, char* path)
{
  srand((unsigned int)time(NULL));

  config_init();    // destroy 2
  player_init();    // destroy 3
  visible_init();   // destroy 4
  callbacks_init(); // destroy 5

  // init paths

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL)
  {
    printf("Current working dir: %s %s\n", cwd, path);
  }
  else
  {
    perror("getcwd() error");
  }

  char* top_path = path_new_normalize(cwd, NULL);
  char* wrk_path = path_new_normalize(path, NULL); // REL 0
#ifdef __linux__
  char* res_path = zm.res_par ? path_new_normalize(zm.res_par, wrk_path) : cstr_new_cstring("/usr/share/zenmedia"); // REL 1
#else
  char* res_path = zm.res_par ? path_new_normalize(zm.res_par, wrk_path) : cstr_new_cstring("/usr/local/share/zenmedia"); // REL 1
#endif
  char* cfgdir_path = zm.cfg_par ? path_new_normalize(zm.cfg_par, wrk_path) : path_new_normalize("~/.config/zenmedia", getenv("HOME")); // REL 2
  char* css_path    = path_new_append(res_path, "main.css");                                                                            // REL 3
  char* html_path   = path_new_append(res_path, "main.html");                                                                           // REL 4
  char* font_path   = path_new_append(res_path, "Baloo.ttf");                                                                           // REL 5
  char* cfg_path    = path_new_append(cfgdir_path, "config.kvl");                                                                       // REL 6
  char* rec_path    = zm.rec_par ? path_new_normalize(zm.rec_par, wrk_path) : NULL;                                                     // REL 7
  char* rep_path    = zm.rep_par ? path_new_normalize(zm.rep_par, wrk_path) : NULL;                                                     // REL 8

  // print path info to console

  printf("top path  : %s\n", top_path);
  printf("working path  : %s\n", wrk_path);
  printf("config path   : %s\n", cfg_path);
  printf("resource path : %s\n", res_path);
  printf("css path      : %s\n", css_path);
  printf("html path     : %s\n", html_path);
  printf("font path     : %s\n", font_path);

  // init config

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

  // init recording/playing

  if (zm.rec_par) evrec_init_recorder(rec_path); // destroy 6
  if (zm.rep_par) evrec_init_player(rep_path);   // destroy 7

  // load ui from descriptors

  ui_init(width, height); // destroy 8

  // load current directoru

  load_directory();

  // init cursor if replay

  if (zm.rep_par) ui_add_cursor();

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
}

void update(ev_t ev)
{
  if (ev.type == EV_TIME)
  {
    ui_play_update();
    // if (!zm.pdfbmp) zm.pdfbmp = pdf_render("/home/milgra/Projects/mmfm/ajanlat.pdf");
    // ui_visualizer_show_image(zm.pdfbmp); // show pdf

    if (zm.rep_par)
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
    if (zm.rec_par)
    {
      evrec_record(ev);
      if (ev.type == EV_KDOWN && ev.keycode == SDLK_PRINTSCREEN) save_screenshot(ev.time);
    }
  }

  // in case of replay only send time events
  if (!zm.rep_par || ev.type == EV_TIME) ui_manager_event(ev);
}

// render, called once per frame

void render(uint32_t time)
{
  ui_manager_render(time);
}

void destroy()
{

  if (zm.rep_par) evrec_destroy(); // destroy 7
  if (zm.rec_par) evrec_destroy(); // destroy 6
  callbacks_destroy();             // destroy 5
  visible_destroy();               // destroy 4
  player_destroy();                // destroy 3
  config_destroy();                // destroy 2
  wm_destroy();                    // destroy 0

  ui_destroy(); // destroy 8

  if (zm.cfg_par) REL(zm.cfg_par); // REL 0
  if (zm.res_par) REL(zm.res_par); // REL 1
  if (zm.rec_par) REL(zm.rec_par); // REL 2
  if (zm.rep_par) REL(zm.rep_par); // REL 3
  if (zm.frm_par) REL(zm.frm_par); // REL 4

#ifdef DEBUG
  mem_stats();
#endif
}

int main(int argc, char* argv[])
{
  printf("MultiMedia File Manager v" MMFM_VERSION " by Milan Toth ( www.milgra.com )\n");

  zc_log_use_colors(isatty(STDERR_FILENO));
  zc_log_level_info();

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

  int option       = 0;
  int option_index = 0;

  while ((option = getopt_long(argc, argv, "vhr:s:p:c:f:", long_options, &option_index)) != -1)
  {
    switch (option)
    {
    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
    case 'c': zm.cfg_par = cstr_new_cstring(optarg); break; // REL 0
    case 'r': zm.res_par = cstr_new_cstring(optarg); break; // REL 1
    case 's': zm.rec_par = cstr_new_cstring(optarg); break; // REL 2
    case 'p': zm.rep_par = cstr_new_cstring(optarg); break; // REL 3
    case 'f': zm.frm_par = cstr_new_cstring(optarg); break; // REL 4
    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
    }
  }

  wm_init(init, update, render, destroy, zm.frm_par); // destroy 0
  return 0;
}

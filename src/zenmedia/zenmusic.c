#include "callbacks.c"
#include "coder.c"
#include "config.c"
#include "database.c"
#include "evrecorder.c"
#include "files.c"
#include "library.c"
#include "player.c"
#include "remote.c"
#include "tg_css.c"
#include "ui.c"
#include "ui_compositor.c"
#include "ui_filter_bar.c"
#include "ui_lib_init_popup.c"
#include "ui_manager.c"
#include "ui_play_controls.c"
#include "ui_song_infos.c"
#include "ui_songlist.c"
#include "visible.c"
#include "wm_connector.c"
#include "wm_event.c"
#include "zc_callback.c"
#include "zc_channel.c"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_log.c"
#include "zc_map.c"
#include "zc_string.c"
#include <SDL.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void init(int width, int height, char* path);
void update(ev_t ev);
void render(uint32_t time);
void destroy();

void load_directory();
void on_change_remote(void* userdata, void* data);
void on_change_library(void* userdata, void* data);
void on_change_organize(void* userdata, void* data);
void get_analyzed_songs();
void get_remote_events();
void update_player();
void save_screenshot(uint32_t time);

struct
{
  ch_t* lib_ch; // library channel
  ch_t* rem_ch; // remote channel

  char* cfg_par; // config path parameter
  char* res_par; // resources path parameter

  char* rec_par; // record parameter
  char* rep_par; // replay parameter

  char* frm_par; // frame parameter

  view_t* rep_cur; // replay cursor
} zm = {0};

int main(int argc, char* argv[])
{
  printf("Zen Music v%i.%i beta by Milan Toth\n", VERSION, BUILD);

  const struct option long_options[] =
      {
          {"resources", optional_argument, 0, 'r'},
          {"record", optional_argument, 0, 's'},
          {"replay", optional_argument, 0, 'p'},
          {"config", optional_argument, 0, 'c'},
          {"frame", optional_argument, 0, 'f'},
          {0, 0, 0, 0},
      };

  int option       = 0;
  int option_index = 0;

  while ((option = getopt_long(argc, argv, "r:s:p:c:f:", long_options, &option_index)) != -1)
  {
    if (option != '?') printf("parsing option %c value: %s\n", option, optarg);
    if (option == 'c') zm.cfg_par = cstr_new_cstring(optarg); // REL 0
    if (option == 'r') zm.res_par = cstr_new_cstring(optarg); // REL 1
    if (option == 's') zm.rec_par = cstr_new_cstring(optarg); // REL 2
    if (option == 'p') zm.rep_par = cstr_new_cstring(optarg); // REL 3
    if (option == 'f') zm.frm_par = cstr_new_cstring(optarg); // REL 4
    if (option == '?')
    {
      printf("-c --config= [config file] \t use config file for session\n");
      printf("-r --resources= [resources folder] \t use resources dir for session\n");
      printf("-s --record= [recorder file] \t record session to file\n");
      printf("-p --replay= [recorder file] \t replay session from file\n");
      printf("-f --frame= [widthxheight] \t initial window dimension\n");
    }
  }

  wm_init(init, update, render, destroy, zm.frm_par); // destroy 0
  return 0;
}

void init(int width, int height, char* path)
{
  srand((unsigned int)time(NULL));

  zm.lib_ch = ch_new(100); // REL -1 // comm channel for library entries
  zm.rem_ch = ch_new(10);  // REL -2 // remote channel

  db_init();        // destroy 1
  config_init();    // destroy 2
  player_init();    // destroy 3
  visible_init();   // destroy 4
  callbacks_init(); // destroy 5

  // init callbacks

  cb_t* change_remote   = cb_new(on_change_remote, NULL);
  cb_t* change_library  = cb_new(on_change_library, NULL);
  cb_t* change_organize = cb_new(on_change_organize, NULL);

  callbacks_set("on_change_remote", change_remote);
  callbacks_set("on_change_library", change_library);
  callbacks_set("on_change_organize", change_organize);

  REL(change_remote);
  REL(change_library);
  REL(change_organize);

  // init paths

  //char* wrk_path = cstr_new_path_normalize(path, NULL); // REL 0
  char* wrk_path = cstr_new_cstring("/home/milgra/Projects/zenmedia/tst");
#ifdef __linux__
  char* res_path = zm.res_par ? cstr_new_path_normalize(zm.res_par, wrk_path) : cstr_new_cstring("/usr/share/zenmedia"); // REL 1
#else
  char* res_path = zm.res_par ? cstr_new_path_normalize(zm.res_par, wrk_path) : cstr_new_cstring("/usr/local/share/zenmedia"); // REL 1
#endif
  char* cfgdir_path = zm.cfg_par ? cstr_new_path_normalize(zm.cfg_par, wrk_path) : cstr_new_path_normalize("~/.config/zenmedia", getenv("HOME")); // REL 2
  char* css_path    = cstr_new_path_append(res_path, "main.css");                                                                                 // REL 3
  char* html_path   = cstr_new_path_append(res_path, "main.html");                                                                                // REL 4
  char* font_path   = cstr_new_path_append(res_path, "Baloo.ttf");                                                                                // REL 5
  char* cfg_path    = cstr_new_path_append(cfgdir_path, "config.kvl");                                                                            // REL 6
  char* rec_path    = zm.rec_par ? cstr_new_path_normalize(zm.rec_par, wrk_path) : NULL;                                                          // REL 7
  char* rep_path    = zm.rep_par ? cstr_new_path_normalize(zm.rep_par, wrk_path) : NULL;                                                          // REL 8

  // print path info to console

  printf("working path     : %s\n", wrk_path);
  printf("config path   : %s\n", cfg_path);
  printf("resource path : %s\n", res_path);
  printf("css path      : %s\n", css_path);
  printf("html path     : %s\n", html_path);
  printf("font path     : %s\n", font_path);

  // init config

  config_set("remote_enabled", "false");
  config_set("remote_port", "23723");
  config_set("organize_lib", "false");
  config_set("dark_mode", "false");
  config_set("res_path", res_path);

  // read config, it overwrites defaults if exists

  config_read(cfg_path);

  // init non-configurable defaults

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

  // start listening for remote control events if set

  if (config_get("remote_enabled") && config_get_bool("remote_enabled")) remote_listen(zm.rem_ch, config_get_int("remote_port")); // CLOSE 0

  // load current directoru

  load_directory();

  if (zm.rec_par) printf("***RECORDING SESSION***\n");

  // init cursor if replay

  if (zm.rep_par)
  {
    printf("***REPLAYING SESSION***\n");
    zm.rep_cur                          = view_new("rep_cur", ((r2_t){10, 10, 10, 10}));
    zm.rep_cur->exclude                 = 0;
    zm.rep_cur->layout.background_color = 0xFF000099;
    zm.rep_cur->needs_touch             = 0;
    tg_css_add(zm.rep_cur);
    ui_manager_add_to_top(zm.rep_cur);
  }

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
    get_analyzed_songs();
    get_remote_events();
    ui_play_update();

    if (zm.rep_par)
    {
      // get recorded events
      ev_t* recev = NULL;
      while ((recev = evrec_replay(ev.time)) != NULL)
      {
        ui_manager_event(*recev);

        view_set_frame(zm.rep_cur, (r2_t){recev->x, recev->y, 10, 10});

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
  remote_close(); // CLOSE 0

  if (zm.rep_par) evrec_destroy(); // destroy 7
  if (zm.rec_par) evrec_destroy(); // destroy 6
  callbacks_destroy();             // destroy 5
  visible_destroy();               // destroy 4
  player_destroy();                // destroy 3
  config_destroy();                // destroy 2
  db_destroy();                    // destroy 1
  wm_destroy();                    // destroy 0

  ui_destroy(); // destroy 8

  if (zm.cfg_par) REL(zm.cfg_par); // REL 0
  if (zm.res_par) REL(zm.res_par); // REL 1
  if (zm.rec_par) REL(zm.rec_par); // REL 2
  if (zm.rep_par) REL(zm.rep_par); // REL 3
  if (zm.frm_par) REL(zm.frm_par); // REL 4

  REL(zm.lib_ch); // REL -1
  REL(zm.rem_ch); // REL -2

#ifdef DEBUG
  mem_stats();
#endif
}

void load_directory()
{
  assert(config_get("wrk_path") != NULL);

  //  db_reset();
  // db_read(config_get("lib_path"));

  map_t* files = MNEW();                         // REL 0
  lib_read_files(config_get("wrk_path"), files); // read all files under library path

  printf("FILES\n");
  mem_describe(files, 0);

  visible_set_files(files);

  visible_set_sortfield("basename", 0);

  ui_songlist_refresh();

  /* db_update(files);                              // remove deleted files from db, remove existing files from files */

  /* if (files->count > 0) */
  /* { */
  /*   LOG("new files detected : %i", files->count); */
  /*   lib_analyze_files(zm.lib_ch, files); // start analyzing new entries */
  /* } */

  // visible_set_sortfield("meta/artist", 0);

  REL(files); // REL 0
}

void on_change_remote(void* userdata, void* data)
{
  if (config_get_bool("remote_enabled"))
    remote_listen(zm.rem_ch, config_get_int("remote_port"));
  else
    remote_close();
}

void on_change_library(void* userdata, void* data)
{
  char* new_path = data;
  char* lib_path = NULL;

  // construct path if needed

  lib_path = cstr_new_path_normalize(new_path, config_get("wrk_path")); // REL 0

  printf("new path %s lib path %s\n", new_path, lib_path);

  // change library if exists

  if (files_path_exists(lib_path))
  {
    config_set("lib_path", lib_path);
    config_write(config_get("cfg_path"));

    load_directory();

    //    ui_lib_init_popup_hide();
  }
  /* else */
  /*   ui_lib_init_popup_show("Location doesn't exists, please enter valid location."); */

  REL(lib_path); // REL 0
}

void on_change_organize(void* userdata, void* data)
{
  if (config_get_bool("organize_lib"))
  {
    int succ = db_organize(config_get("lib_path"), db_get_db());
    if (succ == 0) db_write(config_get("lib_path"));
    //    ui_songlist_refresh();
  }
}

void get_analyzed_songs()
{
  map_t* entry;

  // get analyzed song entries

  while ((entry = ch_recv(zm.lib_ch))) // REL 0
  {
    char* path = MGET(entry, "file/path");

    if (strcmp(path, "//////") != 0)
    {
      // store entry in db

      db_add_entry(path, entry);

      if (db_count() % 100 == 0)
      {
        // filter and sort current db and show in ui partial analysis result

        visible_set_sortfield("meta/artist", 0);
        // ui_songlist_refresh();
      }
    }
    else
    {
      db_write(config_get("lib_path"));

      if (config_get_bool("organize_lib"))
      {
        // organize db if needed

        int succ = db_organize(config_get("lib_path"), db_get_db());
        if (succ == 0) db_write(config_get("lib_path"));
      }

      visible_set_sortfield("meta/artist", 0);
      // ui_songlist_refresh();
    }

    // cleanup, ownership was passed with the channel from analyzer

    REL(entry); // REL 0
  }
}

void get_remote_events()
{
  char* buffer = NULL;
  if ((buffer = ch_recv(zm.rem_ch)))
  {
    if (buffer[0] == '0') ui_play_pause();
    if (buffer[0] == '1') ui_play_prev();
    if (buffer[0] == '2') ui_play_next();
  }
}

void save_screenshot(uint32_t time)
{
  if (config_get("lib_path"))
  {
    static int cnt    = 0;
    view_t*    root   = ui_manager_get_root();
    r2_t       frame  = root->frame.local;
    bm_t*      screen = bm_new(frame.w, frame.h); // REL 0

    // remove cursor for screenshot to remain identical

    if (zm.rep_par)
    {
      ui_manager_remove(zm.rep_cur);
      ui_manager_render(time);
      ui_manager_add_to_top(zm.rep_cur);
    }

    ui_compositor_render_to_bmp(screen);

    char* name    = cstr_new_format(20, "screenshot%.3i.png", cnt++);   // REL 1
    char* path    = cstr_new_path_append(config_get("lib_path"), name); // REL 2
    bm_t* flipped = bm_new_flip_y(screen);                              // REL 3

    coder_write_png(path, flipped);

    REL(flipped); // REL 3
    REL(name);    // REL 2
    REL(path);    // REL 1
    REL(screen);  // REL 0

    if (zm.rep_par) view_set_frame(zm.rep_cur, frame); // full screen cursor to indicate screenshot
  }
}

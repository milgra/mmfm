#include "callbacks.c"
#include "coder.c"
#include "config.c"
#include "evrecorder.c"
#include "files.c"
#include "library.c"
#include "player.c"
#include "tg_css.c"
#include "ui.c"
#include "ui_compositor.c"
#include "ui_filelist.c"
#include "ui_filter_bar.c"
#include "ui_lib_init_popup.c"
#include "ui_manager.c"
#include "ui_play_controls.c"
#include "ui_song_infos.c"
#include "ui_visualizer.c"
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
#include <mupdf/fitz.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void init(int width, int height, char* path);
void update(ev_t ev);
void render(uint32_t time);
void destroy();

void load_directory();
void update_player();
void save_screenshot(uint32_t time);

struct
{
  char* cfg_par; // config path parameter
  char* res_par; // resources path parameter

  char* rec_par; // record parameter
  char* rep_par; // replay parameter

  char* frm_par; // frame parameter

  view_t* rep_cur; // replay cursor

  bm_t* pdfbmp;
} zm = {0};

void renderpdf(char* filename)
{
  char*        input;
  float        zoom, rotate;
  int          page_number, page_count;
  fz_context*  ctx;
  fz_document* doc;
  fz_pixmap*   pix;
  fz_matrix    ctm;
  int          x, y;

  /* if (argc < 3) */
  /* { */
  /*   fprintf(stderr, "usage: example input-file page-number [ zoom [ rotate ] ]\n"); */
  /*   fprintf(stderr, "\tinput-file: path of PDF, XPS, CBZ or EPUB document to open\n"); */
  /*   fprintf(stderr, "\tPage numbering starts from one.\n"); */
  /*   fprintf(stderr, "\tZoom level is in percent (100 percent is 72 dpi).\n"); */
  /*   fprintf(stderr, "\tRotation is in degrees clockwise.\n"); */
  /*   return EXIT_FAILURE; */
  /* } */

  input       = filename;
  page_number = 0;
  zoom        = 100.0;
  rotate      = 0.0;

  /* Create a context to hold the exception stack and various caches. */
  ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
  if (!ctx)
  {
    fprintf(stderr, "cannot create mupdf context\n");
  }

  /* Register the default file types to handle. */
  fz_try(ctx)
      fz_register_document_handlers(ctx);
  fz_catch(ctx)
  {
    fprintf(stderr, "cannot register document handlers: %s\n", fz_caught_message(ctx));
    fz_drop_context(ctx);
  }

  /* Open the document. */
  fz_try(ctx)
      doc = fz_open_document(ctx, input);
  fz_catch(ctx)
  {
    fprintf(stderr, "cannot open document: %s\n", fz_caught_message(ctx));
    fz_drop_context(ctx);
  }

  /* Count the number of pages. */
  fz_try(ctx)
      page_count = fz_count_pages(ctx, doc);
  fz_catch(ctx)
  {
    fprintf(stderr, "cannot count number of pages: %s\n", fz_caught_message(ctx));
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);
  }

  if (page_number < 0 || page_number >= page_count)
  {
    fprintf(stderr, "page number out of range: %d (page count %d)\n", page_number + 1, page_count);
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);
  }

  /* Compute a transformation matrix for the zoom and rotation desired. */
  /* The default resolution without scaling is 72 dpi. */
  ctm = fz_scale(zoom / 100, zoom / 100);
  ctm = fz_pre_rotate(ctm, rotate);

  /* Render page to an RGB pixmap. */
  fz_try(ctx)
      pix = fz_new_pixmap_from_page_number(ctx, doc, page_number, ctm, fz_device_rgb(ctx), 0);
  fz_catch(ctx)
  {
    fprintf(stderr, "cannot render page: %s\n", fz_caught_message(ctx));
    fz_drop_document(ctx, doc);
    fz_drop_context(ctx);
  }

  /* Print image data in ascii PPM format. */
  printf("P3\n");
  printf("w %d h %d s %td\n", pix->w, pix->h, pix->stride);
  printf("255\n");

  zm.pdfbmp     = bm_new(pix->w, pix->h);
  uint8_t* data = zm.pdfbmp->data;

  for (y = 0; y < pix->h; ++y)
  {
    unsigned char* p = &pix->samples[y * pix->stride];
    for (x = 0; x < pix->w; ++x)
    {
      data[0] = p[0];
      data[1] = p[1];
      data[2] = p[2];
      data[3] = 255;
      data += 4;
      /* if (x > 0) */
      /*   printf("  "); */
      /* if (p[0] < 255) */
      /*   printf("%3d %3d %3d", p[0], p[1], p[2]); */
      p += pix->n;
    }
    // printf("\n");
  }

  /* Clean up. */
  fz_drop_pixmap(ctx, pix);
  fz_drop_document(ctx, doc);
  fz_drop_context(ctx);
}

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

  renderpdf("/home/milgra/Projects/zenmedia/ajanlat.pdf");

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

  char* top_path = cstr_new_path_normalize(cwd, NULL);
  char* wrk_path = cstr_new_path_normalize(path, NULL); // REL 0
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
    ui_play_update();
    ui_visualizer_show_image(zm.pdfbmp); // show pdf

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

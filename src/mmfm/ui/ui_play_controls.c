#ifndef ui_play_controls_h
#define ui_play_controls_h

#include "view.c"

void ui_play_controls_attach(view_t* baseview);
void ui_play_controls_detach();

void ui_play_next();
void ui_play_prev();
void ui_play_pause();
void ui_play_index(int index);

void ui_play_update_position(float ratio);
// void ui_play_update_volume(float ratio);

void ui_play_jump_to();
void ui_play_update();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "config.c"
#include "player.c"
#include "tg_knob.c"
#include "ui_filelist.c"
#include "ui_meta_view.c"
#include "ui_song_infos.c"
#include "ui_visualizer.c"
#include "vh_button.c"
#include "vh_knob.c"
#include "visible.c"
#include "zc_cstring.c"
#include "zc_log.c"
#include <limits.h>
#include <time.h>

struct ui_play_controls_t
{
  double  last_step;
  char    shuffle;
  view_t* playbtn;
  /* view_t* mutebtn; */
  /* view_t* volknob; */
  view_t* seekknob;
  int     lastindex;
  char*   current_path;
} uipc;

void ui_play_on_button_down(void* userdata, void* data);
void ui_play_on_play_button_down(view_t* view);
/* void ui_play_on_mute_button_down(view_t* view); */
void ui_play_on_position_change(view_t* view, float angle);
/* void ui_play_on_volume_change(view_t* view, float angle); */

void ui_play_controls_attach(view_t* baseview)
{
  uipc.seekknob = view_get_subview(baseview, "seekknob");

  if (uipc.seekknob)
  {

    uipc.playbtn = view_get_subview(baseview, "playbtn");
    /* uipc.volknob  = view_get_subview(baseview, "volknob"); */
    /* uipc.mutebtn  = view_get_subview(baseview, "mutebtn"); */

    cb_t* cb_btn_press = cb_new(ui_play_on_button_down, NULL); // REL 0

    vh_button_add(view_get_subview(baseview, "nextbtn"), VH_BUTTON_NORMAL, cb_btn_press);
    vh_button_add(view_get_subview(baseview, "prevbtn"), VH_BUTTON_NORMAL, cb_btn_press);
    vh_button_add(view_get_subview(baseview, "shufflebtn"), VH_BUTTON_TOGGLE, cb_btn_press);

    tg_knob_add(uipc.seekknob);
    vh_knob_add(uipc.seekknob, ui_play_on_position_change, ui_play_on_play_button_down);

    /* tg_knob_add(uipc.volknob); */
    /* vh_knob_add(uipc.volknob, ui_play_on_volume_change, ui_play_on_mute_button_down); */

    vh_button_add(uipc.playbtn, VH_BUTTON_TOGGLE, cb_btn_press);
    /* vh_button_add(uipc.mutebtn, VH_BUTTON_TOGGLE, cb_btn_press); */

    // ui_play_update_volume(0.9);

    REL(cb_btn_press); // REL 0
  }
  else
    zc_log_debug("seekknob not found");
}

void ui_play_controls_detach()
{
}

void ui_play_update_position(float ratio)
{
  tg_knob_set_angle(uipc.seekknob, ratio * 6.28 - 3.14 / 2.0);
}

/* void ui_play_update_volume(float ratio) */
/* { */
/*   tg_knob_set_angle(uipc.volknob, ratio * 6.28 - 3.14 / 2.0); */
/* } */

void ui_play_index(int index)
{
  uipc.lastindex = index;
  if (uipc.lastindex < 0) uipc.lastindex = 0;
  if (uipc.lastindex < visible_song_count())
  {

    // ui_song_infos_show(uipc.lastindex);
    vec_t* songs   = visible_get_songs();
    map_t* songmap = songs->data[uipc.lastindex];

    mem_describe(songmap, 0);

    uipc.current_path = MGET(songmap, "path");

    player_play(uipc.current_path);
    player_set_volume(0.9);

    ui_meta_view_show_file(songmap);

    vh_button_set_state(uipc.playbtn, VH_BUTTON_DOWN);
  }
}

void ui_play_next()
{
  if (uipc.shuffle)
    ui_play_index(rand() % visible_song_count());
  else
    ui_play_index(uipc.lastindex + 1);

  ui_filelist_select_and_show(uipc.lastindex);
}

void ui_play_prev()
{
  if (uipc.shuffle)
    ui_play_index(rand() % visible_song_count());
  else
    ui_play_index(uipc.lastindex - 1);

  ui_filelist_select_and_show(uipc.lastindex);
}

void ui_play_pause()
{
  int state = player_toggle_pause();
  ui_filelist_toggle_pause(state);
  vh_button_set_state(uipc.playbtn, state ? VH_BUTTON_UP : VH_BUTTON_DOWN);
}

void ui_toggle_shuffle()
{
  uipc.shuffle = !uipc.shuffle;
}

void ui_play_on_play_button_down(view_t* view)
{
  if (player_toggle_pause() < 0)
  {
    ui_play_index(0);
  }
  // ui_toggle_pause();
}

/* void ui_play_on_mute_button_down(view_t* view) */
/* { */
/*   player_toggle_mute(); */
/* } */

void ui_play_on_position_change(view_t* view, float angle)
{
  float ratio = 0.0;
  if (angle > 0 && angle < 3.14 * 3 / 2)
  {
    ratio = angle / 6.28 + 0.25;
  }
  else if (angle > 3.14 * 3 / 2)
  {
    ratio = (angle - (3.14 * 3 / 2)) / 6.28;
  }

  player_set_position(ratio);
}

/* void ui_play_on_volume_change(view_t* view, float angle) */
/* { */
/*   float ratio = 0.0; */
/*   if (angle > 0 && angle < 3.14 * 3 / 2) */
/*   { */
/*     ratio = angle / 6.28 + 0.25; */
/*   } */
/*   else if (angle > 3.14 * 3 / 2) */
/*   { */
/*     ratio = (angle - (3.14 * 3 / 2)) / 6.28; */
/*   } */

/*   player_set_volume(ratio); */
/* } */

void ui_play_on_button_down(void* userdata, void* data)
{
  char* id = ((view_t*)data)->id;

  if (strcmp(id, "playbtn") == 0) ui_play_on_play_button_down(NULL);
  /* if (strcmp(id, "mutebtn") == 0) ui_play_on_mute_button_down(NULL); */
  if (strcmp(id, "nextbtn") == 0) ui_play_next();
  if (strcmp(id, "prevbtn") == 0) ui_play_index(uipc.lastindex - 1);
  if (strcmp(id, "shufflebtn") == 0) ui_toggle_shuffle();
}

void ui_play_jump_to()
{
  ui_filelist_select_and_show(uipc.lastindex);
}

void ui_play_update()
{
  double phead = player_time();

  if (phead > 0.0)
  {
    if (floor(phead) != uipc.last_step)
    {
      uipc.last_step = floor(phead);

      double posratio = phead / player_duration();
      /*     double volratio = player_volume(); */

      /*     ui_song_infos_update_time(uipc.last_step, player_duration() - uipc.last_step, player_duration()); */
      ui_play_update_position(posratio);
      /*     // ui_play_update_volume(volratio); */
    }

    ui_visualizer_update();
    ui_visualizer_update_video();
  }

  /* // update player */

  // int finished = player_refresh();
  /* if (finished) ui_play_next(); */
}

#endif

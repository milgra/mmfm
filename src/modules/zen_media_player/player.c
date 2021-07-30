/*
  This module is based on ffplay.c so all bugfixes/updates to ffplay should be introduced to this module.
 */

#ifndef player_h
#define player_h

#include "zc_bitmap.c"
#include "zc_map.c"

void  player_init();
void  player_destroy();
void  player_play(char* path);
char* player_get_path();

int  player_toggle_pause();
void player_toggle_mute();

void player_set_volume(float ratio);
void player_set_position(float ratio);

double player_time();
double player_volume();
double player_duration();

void player_draw_video(bm_t* bm, int edge);
void player_draw_video_to_texture(int index, int w, int h);
void player_draw_waves(int channel, bm_t* bm, int edge);
void player_draw_rdft(int channel, bm_t* bm, int edge);
int  player_refresh();
int  player_finished();

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "render.c"
#include "strcomm.c"
#include "stream.c"
#include "zc_cstring.c"
#include "zc_graphics.c"
#include "zc_log.c"
#include <assert.h>

static AVInputFormat* file_iformat;

VideoState* is             = NULL;
double      remaining_time = 0.0;
char*       player_path    = NULL;
int         player_ended   = 0;

void player_init()
{
  av_init_packet(&flush_pkt);
  flush_pkt.data = (uint8_t*)&flush_pkt;
}

void player_destroy()
{
  if (is != NULL) stream_close(is);
}

char* player_get_path()
{
  return player_path;
}

void player_play(char* path)
{
  player_path = path;
  if (is != NULL) stream_close(is);
  is           = stream_open(path, file_iformat);
  player_ended = 0;
}

int player_toggle_pause()
{
  if (is)
  {
    stream_toggle_pause(is);
    is->step = 0;

    return is->paused;
  }

  return -1;
}

void player_toggle_mute()
{
  if (is)
  {
    is->muted = !is->muted;
  }
}

double player_time()
{
  if (is != NULL)
  {
    return get_master_clock(is);
  }
  else
    return 0.0;
}

double player_duration()
{
  if (is != NULL)
  {
    return is->duration;
  }
  else
    return 0.0;
}

void player_set_position(float ratio)
{
  if (is)
  {
    int newpos = (int)player_duration() * ratio;
    int diff   = (int)player_time() - newpos;
    /* printf("ratio %f\n", ratio); */
    /* printf("duration %f\n", player_duration()); */
    /* printf("newpos %i\n", newpos); */
    stream_seek(is, (int64_t)(newpos * AV_TIME_BASE), (int64_t)(diff * AV_TIME_BASE), 0);
  }
}

double player_volume()
{
  if (is != NULL)
  {
    return (float)is->audio_volume / (float)SDL_MIX_MAXVOLUME;
  }
  else
    return 1.0;
}

void player_set_volume(float ratio)
{
  /* double volume_level = is->audio_volume ? (20 * log(is->audio_volume / (double)SDL_MIX_MAXVOLUME) / log(10)) : -1000.0; */
  /* int    new_volume   = lrint(SDL_MIX_MAXVOLUME * pow(10.0, (volume_level + sign * step) / 20.0)); */
  /* is->audio_volume    = av_clip(is->audio_volume == new_volume ? (is->audio_volume + sign) : new_volume, 0, SDL_MIX_MAXVOLUME); */
  is->audio_volume = (int)((float)SDL_MIX_MAXVOLUME * ratio);
}

int player_refresh()
{
  if (is != NULL)
  {
    if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
    {
      remaining_time = 0.01;
      video_refresh(is, &remaining_time, 0);
    }

    if (is->play_finished)
    {
      stream_close(is);
      is           = NULL;
      player_ended = 1;
      return 1;
    }
  }

  return 0;
}

int player_finished()
{
  return player_ended;
}

void player_draw_video_to_texture(int index, int w, int h)
{
  if (is != NULL)
  {
    if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
    {
      video_show(is, index, w, h, NULL, 0);
    }
  }
}

void player_draw_video(bm_t* bm, int edge)
{
  if (is != NULL)
  {
    if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
    {
      video_show(is, 0, bm->w - 2 * edge, bm->h - 2 * edge, bm, 3);
    }
  }
}

void player_draw_waves(int channel, bm_t* bm, int edge)
{
  if (is != NULL)
  {
    if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
    {
      render_draw_waves(is, channel, bm, edge);
    }
  }
}

void player_draw_rdft(int channel, bm_t* bm, int edge)
{
  if (is != NULL)
  {
    if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
    {
      render_draw_rdft(is, channel, bm, edge);
    }
  }
}

#endif

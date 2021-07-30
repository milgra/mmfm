
#ifndef render_h
#define render_h

#include "zc_bitmap.c"

void video_show(void* opaque, int index, int w, int h, bm_t* bitmap, int edge);
void render_draw_waves(void* opaque, int channel, bm_t* bitmap, int edge);
void render_draw_rdft(void* opaque, int channel, bm_t* bitmap, int edge);
void video_refresh(void* opaque, double* remaining_time, int index);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "gl_connector.c"
#include "libavutil/bprint.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "options.c"
#include "strcomm.c"
#include "video.c"
#include "zc_graphics.c"
#include <GL/glew.h>

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be vduplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN 0.900
#define EXTERNAL_CLOCK_SPEED_MAX 1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001

#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

double rdftspeed = 0.02;

int display_disable;

static const struct TextureFormatEntry
{
  enum AVPixelFormat format;
  int                texture_fmt;
} sdl_texture_format_map[] = {
    {AV_PIX_FMT_RGB8, SDL_PIXELFORMAT_RGB332},
    {AV_PIX_FMT_RGB444, SDL_PIXELFORMAT_RGB444},
    {AV_PIX_FMT_RGB555, SDL_PIXELFORMAT_RGB555},
    {AV_PIX_FMT_BGR555, SDL_PIXELFORMAT_BGR555},
    {AV_PIX_FMT_RGB565, SDL_PIXELFORMAT_RGB565},
    {AV_PIX_FMT_BGR565, SDL_PIXELFORMAT_BGR565},
    {AV_PIX_FMT_RGB24, SDL_PIXELFORMAT_RGB24},
    {AV_PIX_FMT_BGR24, SDL_PIXELFORMAT_BGR24},
    {AV_PIX_FMT_0RGB32, SDL_PIXELFORMAT_RGB888},
    {AV_PIX_FMT_0BGR32, SDL_PIXELFORMAT_BGR888},
    {AV_PIX_FMT_NE(RGB0, 0BGR), SDL_PIXELFORMAT_RGBX8888},
    {AV_PIX_FMT_NE(BGR0, 0RGB), SDL_PIXELFORMAT_BGRX8888},
    {AV_PIX_FMT_RGB32, SDL_PIXELFORMAT_ARGB8888},
    {AV_PIX_FMT_RGB32_1, SDL_PIXELFORMAT_RGBA8888},
    {AV_PIX_FMT_BGR32, SDL_PIXELFORMAT_ABGR8888},
    {AV_PIX_FMT_BGR32_1, SDL_PIXELFORMAT_BGRA8888},
    {AV_PIX_FMT_YUV420P, SDL_PIXELFORMAT_IYUV},
    {AV_PIX_FMT_YUYV422, SDL_PIXELFORMAT_YUY2},
    {AV_PIX_FMT_UYVY422, SDL_PIXELFORMAT_UYVY},
    {AV_PIX_FMT_NONE, SDL_PIXELFORMAT_UNKNOWN},
};

static inline int compute_mod(int a, int b)
{
  return a < 0 ? a % b + b : a % b;
}

static void update_video_pts(VideoState* is, double pts, int64_t pos, int serial)
{
  /* update current video pts */
  set_clock(&is->vidclk, pts, serial);
  sync_clock_to_slave(&is->extclk, &is->vidclk);
}

static double compute_target_delay(double delay, VideoState* is)
{
  double sync_threshold, diff = 0;

  /* update delay to follow master synchronisation source */
  if (get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)
  {
    /* if video is slave, we try to correct big delays by
           duplicating or deleting a frame */
    diff = get_clock(&is->vidclk) - get_master_clock(is);

    /* skip or repeat frame. We take into account the
           delay to compute the threshold. I still don't know
           if it is the best guess */
    sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    if (!isnan(diff) && fabs(diff) < is->max_frame_duration)
    {
      if (diff <= -sync_threshold)
        delay = FFMAX(0, delay + diff);
      else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
        delay = delay + diff;
      else if (diff >= sync_threshold)
        delay = 2 * delay;
    }
  }

  av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",
         delay, -diff);

  return delay;
}

static double vp_duration(VideoState* is, Frame* vp, Frame* nextvp)
{
  if (vp->serial == nextvp->serial)
  {
    double duration = nextvp->pts - vp->pts;
    if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
      return vp->duration;
    else
      return duration;
  }
  else
  {
    return 0.0;
  }
}

void check_external_clock_speed(VideoState* is)
{
  if (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
      is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES)
  {
    set_clock_speed(&is->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
  }
  else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
           (is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES))
  {
    set_clock_speed(&is->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
  }
  else
  {
    double speed = is->extclk.speed;
    if (speed != 1.0)
      set_clock_speed(&is->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
  }
}

static void video_audio_display(VideoState* s, int index, bm_t* bitmap, int edge, enum ShowMode showmode)
{
  int     i, i_start, x, y1, y2, y, ys, delay, n, nb_display_channels;
  int     ch, channels, h, h2;
  int64_t time_diff;
  int     rdft_bits, nb_freq;

  int width  = bitmap->w - 2 * edge;
  int height = bitmap->h - 2 * edge;

  for (rdft_bits = 1; (1 << rdft_bits) < 2 * height; rdft_bits++)
    ;
  nb_freq = 1 << (rdft_bits - 1);

  s->show_mode = showmode;

  /* compute display index : center on currently output samples */
  channels            = s->audio_tgt.channels;
  nb_display_channels = channels;
  if (!s->paused)
  {
    int data_used = s->show_mode == SHOW_MODE_WAVES ? width : (2 * nb_freq);
    n             = 2 * channels;
    delay         = s->audio_write_buf_size;
    delay /= n;

    /* to be more precise, we take into account the time spent since
           the last buffer computation */
    if (audio_callback_time)
    {
      time_diff = av_gettime_relative() - audio_callback_time;
      delay -= (time_diff * s->audio_tgt.freq) / 1000000;
    }

    delay += 2 * data_used;
    if (delay < data_used)
      delay = data_used;

    i_start = x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
    if (s->show_mode == SHOW_MODE_WAVES)
    {
      h = INT_MIN;
      for (i = 0; i < 1000; i += channels)
      {
        int idx   = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
        int a     = s->sample_array[idx];
        int b     = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
        int c     = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
        int d     = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
        int score = a - d;
        if (h < score && (b ^ c) < 0)
        {
          h       = score;
          i_start = idx;
        }
      }
    }

    s->last_i_start = i_start;
  }
  else
  {
    i_start = s->last_i_start;
  }

  if (s->show_mode == SHOW_MODE_WAVES)
  {
    gfx_rect(bitmap, edge, edge, width, height, 0x000000FF, 1);

    /* total height for one channel */
    h = height;
    /* graph height / 2 */
    h2 = (h * 9) / 20;

    ch = index;

    {
      i  = i_start + ch;
      y1 = s->ytop + (h / 2); /* position of center line */

      int prevy = 0;
      for (x = 0; x < width; x++)
      {
        y = (s->sample_array[i] * h2) >> 15;
        if (y < 0)
        {
          y  = -y;
          ys = y1 - y;
          y2 = ys;
        }
        else
        {
          ys = y1;
          y2 = ys + y;
        }

        int sy = prevy < y2 ? prevy : y2;
        int hy = prevy < y2 ? y2 - prevy : prevy - y2;
        if (hy == 0) hy = 1;

        gfx_rect(bitmap, s->xleft + x, sy, 1, hy, 0xFFFFFFFF, 1);

        prevy = y2;

        i += channels;
        if (i >= SAMPLE_ARRAY_SIZE)
          i -= SAMPLE_ARRAY_SIZE;
      }
    }
  }
  else
  {
    /* if (realloc_texture(&s->vis_texture, SDL_PIXELFORMAT_ARGB8888, width, height, SDL_BLENDMODE_NONE, 1) < 0) */
    /*   return; */

    nb_display_channels = FFMIN(nb_display_channels, 2);
    if (rdft_bits != s->rdft_bits)
    {
      av_rdft_end(s->rdft);
      av_free(s->rdft_data);
      s->rdft      = av_rdft_init(rdft_bits, DFT_R2C);
      s->rdft_bits = rdft_bits;
      s->rdft_data = av_malloc_array(nb_freq, 4 * sizeof(*s->rdft_data));
    }
    if (!s->rdft || !s->rdft_data)
    {
      av_log(NULL, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n");
      s->show_mode = SHOW_MODE_WAVES;
    }
    else
    {
      FFTSample* data[2];
      SDL_Rect   rect = {.x = s->xpos, .y = 0, .w = 1, .h = height};
      uint32_t*  pixels;
      int        pitch;

      ch = index;
      for (ch = 0; ch < nb_display_channels; ch++)
      {
        data[ch] = s->rdft_data + 2 * nb_freq * ch;
        i        = i_start + ch;
        for (x = 0; x < 2 * nb_freq; x++)
        {
          double w    = (x - nb_freq) * (1.0 / nb_freq);
          data[ch][x] = s->sample_array[i] * (1.0 - w * w);
          i += channels;
          if (i >= SAMPLE_ARRAY_SIZE)
            i -= SAMPLE_ARRAY_SIZE;
        }
        av_rdft_calc(s->rdft, data[ch]);
      }
      /* Least efficient way to do this, we should of course
             * directly access it but it is more than fast enough. */
      /* if (!SDL_LockTexture(s->vis_texture, &rect, (void**)&pixels, &pitch)) */
      /* { */

      pitch = width;
      pitch >>= 2;
      pixels = (uint32_t*)bitmap->data;
      pixels += pitch * height;
      for (y = 0; y < height; y++)
      {
        double w = 1 / sqrt(nb_freq);
        int    a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
        int    b = (nb_display_channels == 2) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
                                           : a;
        a = FFMIN(a, 255);
        b = FFMIN(b, 255);
        pixels -= pitch;
        uint32_t color = ((a << 16) + (b << 8) + ((a + b) >> 1)) << 8 | 0xFF;

        int h = bitmap->h;
        gfx_rect(bitmap, s->xpos + edge, h - edge - y, 1, 1, color, 1);
        gfx_rect(bitmap, s->xpos + edge + 1, 0, 1, h, 0xFFFFFFFF, 1);
        /*}*/
        /* SDL_UnlockTexture(s->vis_texture); */
      }
      /* SDL_RenderCopy(renderer, s->vis_texture, NULL, NULL); */
    }
    if (!s->paused && index == 1)
      s->xpos++;
    if (s->xpos >= width)
      s->xpos = s->xleft;
  }
}

uint8_t* scaledpixels[1];

static unsigned sws_flags = SWS_BICUBIC;

static int upload_texture(SDL_Texture** tex, AVFrame* frame, SDL_Rect rect, struct SwsContext** img_convert_ctx, int index, int w, int h, bm_t* bitmap, int edge)
{
  int ret = 0;

  *img_convert_ctx = sws_getCachedContext(*img_convert_ctx,
                                          frame->width,
                                          frame->height,
                                          frame->format,
                                          w,
                                          h,
                                          AV_PIX_FMT_BGR32,
                                          sws_flags,
                                          NULL,
                                          NULL,
                                          NULL);

  if (*img_convert_ctx != NULL)
  {
    uint8_t* pixels[4];
    int      pitch[4];

    pitch[0] = w * 4;
    sws_scale(*img_convert_ctx,
              (const uint8_t* const*)frame->data,
              frame->linesize,
              0,
              frame->height,
              scaledpixels,
              pitch);

    if (bitmap)
    {
      gfx_insert_rgb(bitmap, scaledpixels[0], w, h, edge, edge);
    }
    else
    {
      gl_upload_to_texture(index, 0, 0, w, h, scaledpixels[0]);
    }
  }
  return ret;
}

static void calculate_display_rect(SDL_Rect*  rect,
                                   int        scr_xleft,
                                   int        scr_ytop,
                                   int        scr_width,
                                   int        scr_height,
                                   int        pic_width,
                                   int        pic_height,
                                   AVRational pic_sar)
{
  AVRational aspect_ratio = pic_sar;
  int64_t    width, height, x, y;

  if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
    aspect_ratio = av_make_q(1, 1);

  aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));

  /* XXX: we suppose the screen has a 1.0 pixel ratio */
  height = scr_height;
  width  = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
  if (width > scr_width)
  {
    width  = scr_width;
    height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
  }
  x       = (scr_width - width) / 2;
  y       = (scr_height - height) / 2;
  rect->x = scr_xleft + x;
  rect->y = scr_ytop + y;
  rect->w = FFMAX((int)width, 1);
  rect->h = FFMAX((int)height, 1);
}

static void video_image_display(VideoState* is, int index, int w, int h, bm_t* bitmap, int edge)
{
  Frame*   vp;
  Frame*   sp = NULL;
  SDL_Rect rect;

  vp = frame_queue_peek_last(&is->pictq);
  if (is->subtitle_st)
  {
    if (frame_queue_nb_remaining(&is->subpq) > 0)
    {
      sp = frame_queue_peek(&is->subpq);

      if (vp->pts >= sp->pts + ((float)sp->sub.start_display_time / 1000))
      {
        if (!sp->uploaded)
        {
          /*           uint8_t* pixels[4]; */
          /*           int      pitch[4]; */
          /*           int      i; */
          /*           if (!sp->width || !sp->height) */
          /*           { */
          /*             sp->width  = vp->width; */
          /*             sp->height = vp->height; */
          /*           } */
          /*           if (realloc_texture(&is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height, SDL_BLENDMODE_BLEND, 1) < 0) */
          /*             return; */

          /*           for (i = 0; i < sp->sub.num_rects; i++) */
          /*           { */
          /*             AVSubtitleRect* sub_rect = sp->sub.rects[i]; */

          /*             sub_rect->x = av_clip(sub_rect->x, 0, sp->width); */
          /*             sub_rect->y = av_clip(sub_rect->y, 0, sp->height); */
          /*             sub_rect->w = av_clip(sub_rect->w, 0, sp->width - sub_rect->x); */
          /*             sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y); */

          /*             is->sub_convert_ctx = sws_getCachedContext(is->sub_convert_ctx, */
          /*                                                        sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8, */
          /*                                                        sub_rect->w, sub_rect->h, AV_PIX_FMT_BGRA, */
          /*                                                        0, NULL, NULL, NULL); */
          /*             if (!is->sub_convert_ctx) */
          /*             { */
          /*               av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n"); */
          /*               return; */
          /*             } */
          /*             if (!SDL_LockTexture(is->sub_texture, (SDL_Rect*)sub_rect, (void**)pixels, pitch)) */
          /*             { */
          /*               sws_scale(is->sub_convert_ctx, (const uint8_t* const*)sub_rect->data, sub_rect->linesize, */
          /*                         0, sub_rect->h, pixels, pitch); */
          /*               SDL_UnlockTexture(is->sub_texture); */
          /*             } */
          /*           } */
          sp->uploaded = 1;
        }
      }
      else
        sp = NULL;
    }
  }

  calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height, vp->sar);

  if (!vp->uploaded)
  {
    if (upload_texture(&is->vid_texture, vp->frame, rect, &is->img_convert_ctx, index, w, h, bitmap, edge) < 0)
      return;
    vp->uploaded = 1;
    vp->flip_v   = vp->frame->linesize[0] < 0;
  }

  /*   set_sdl_yuv_conversion_mode(vp->frame); */
  /*   SDL_RenderCopyEx(renderer, is->vid_texture, NULL, &rect, 0, NULL, vp->flip_v ? SDL_FLIP_VERTICAL : 0); */
  /*   set_sdl_yuv_conversion_mode(NULL); */
  /*   if (sp) */
  /*   { */
  /* #if USE_ONEPASS_SUBTITLE_RENDER */
  /*     SDL_RenderCopy(renderer, is->sub_texture, NULL, &rect); */
  /* #else */
  /*     int    i; */
  /*     double xratio = (double)rect.w / (double)sp->width; */
  /*     double yratio = (double)rect.h / (double)sp->height; */
  /*     for (i = 0; i < sp->sub.num_rects; i++) */
  /*     { */
  /*       SDL_Rect* sub_rect = (SDL_Rect*)sp->sub.rects[i]; */
  /*       SDL_Rect  target   = {.x = rect.x + sub_rect->x * xratio, */
  /*                          .y = rect.y + sub_rect->y * yratio, */
  /*                          .w = sub_rect->w * xratio, */
  /*                          .h = sub_rect->h * yratio}; */
  /*       SDL_RenderCopy(renderer, is->sub_texture, sub_rect, &target); */
  /*     } */
  /* #endif */
  /*   } */
}

static int video_open(VideoState* is)
{
  int w, h;

  /* w = screen_width ? screen_width : default_width; */
  /* h = screen_height ? screen_height : default_height; */

  /* if (!window_title) */
  /*   window_title = input_filename; */
  /* SDL_SetWindowTitle(window, window_title); */

  /* SDL_SetWindowSize(window, w, h); */
  /* SDL_SetWindowPosition(window, screen_left, screen_top); */
  /* if (is_full_screen) */
  /*   SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP); */
  /* SDL_ShowWindow(window); */

  is->width  = 1280;
  is->height = 720;

  printf("video open\n");

  scaledpixels[0] = malloc(1280 * 720 * 4);

  return 0;
}

void video_show(void* opaque, int index, int w, int h, bm_t* bitmap, int edge)
{
  VideoState* is = opaque;

  if (!display_disable && is->pictq.rindex_shown)
  {
    if (is->width != w)
    {
      is->width  = w;
      is->height = h;

      scaledpixels[0] = malloc(w * h * 4);
    }
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderClear(renderer);
    /* if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO) */
    //video_audio_display(is, index);
    /* else if (is->video_st) */
    video_image_display(is, index, w, h, bitmap, edge);
    //SDL_RenderPresent(renderer);
  }
  else
  {
    gfx_rect(bitmap, 0, 0, bitmap->w, bitmap->h, 0x000000FF, 1);
  }
}

void render_draw_waves(void* opaque, int index, bm_t* bitmap, int edge)
{
  VideoState* is = opaque;

  if (!display_disable)
  {
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderClear(renderer);
    /* if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO) */
    video_audio_display(is, index, bitmap, edge, SHOW_MODE_WAVES);
    /* else if (is->video_st) */
    //video_image_display(is, index);
    //SDL_RenderPresent(renderer);
  }
}

void render_draw_rdft(void* opaque, int index, bm_t* bitmap, int edge)
{
  VideoState* is = opaque;

  if (!display_disable)
  {
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderClear(renderer);
    /* if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO) */
    video_audio_display(is, index, bitmap, edge, SHOW_MODE_RDFT);
    /* else if (is->video_st) */
    //video_image_display(is, index);
    //SDL_RenderPresent(renderer);
  }
}

/* called to display each frame */
void video_refresh(void* opaque, double* remaining_time, int index)
{
  VideoState* is = opaque;
  double      time;

  Frame *sp, *sp2;

  if (!is->paused && get_master_sync_type(is) == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
    check_external_clock_speed(is);

  if (!display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st)
  {
    time = av_gettime_relative() / 1000000.0;
    if (is->force_refresh || is->last_vis_time + rdftspeed < time)
    {
      // video_display(is, index);
      is->last_vis_time = time;
    }
    *remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
  }

  if (is->video_st)
  {
  retry:
    if (frame_queue_nb_remaining(&is->pictq) == 0)
    {
      // nothing to do, no picture to display in the queue
    }
    else
    {
      double last_duration, duration, delay;
      Frame *vp, *lastvp;

      /* dequeue the picture */
      lastvp = frame_queue_peek_last(&is->pictq);
      vp     = frame_queue_peek(&is->pictq);

      if (vp->serial != is->videoq.serial)
      {
        frame_queue_next(&is->pictq);
        goto retry;
      }

      if (lastvp->serial != vp->serial)
        is->frame_timer = av_gettime_relative() / 1000000.0;

      if (is->paused)
        goto display;

      /* compute nominal last_duration */
      last_duration = vp_duration(is, lastvp, vp);
      delay         = compute_target_delay(last_duration, is);

      time = av_gettime_relative() / 1000000.0;
      if (time < is->frame_timer + delay)
      {
        *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
        goto display;
      }

      is->frame_timer += delay;
      if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
        is->frame_timer = time;

      SDL_LockMutex(is->pictq.mutex);
      if (!isnan(vp->pts))
        update_video_pts(is, vp->pts, vp->pos, vp->serial);
      SDL_UnlockMutex(is->pictq.mutex);

      if (frame_queue_nb_remaining(&is->pictq) > 1)
      {
        Frame* nextvp = frame_queue_peek_next(&is->pictq);
        duration      = vp_duration(is, vp, nextvp);
        if (!is->step && (framedrop > 0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER)) && time > is->frame_timer + duration)
        {
          is->frame_drops_late++;
          frame_queue_next(&is->pictq);
          goto retry;
        }
      }

      if (is->subtitle_st)
      {
        while (frame_queue_nb_remaining(&is->subpq) > 0)
        {
          sp = frame_queue_peek(&is->subpq);

          if (frame_queue_nb_remaining(&is->subpq) > 1)
            sp2 = frame_queue_peek_next(&is->subpq);
          else
            sp2 = NULL;

          if (sp->serial != is->subtitleq.serial || (is->vidclk.pts > (sp->pts + ((float)sp->sub.end_display_time / 1000))) || (sp2 && is->vidclk.pts > (sp2->pts + ((float)sp2->sub.start_display_time / 1000))))
          {
            if (sp->uploaded)
            {
              int i;
              for (i = 0; i < sp->sub.num_rects; i++)
              {
                AVSubtitleRect* sub_rect = sp->sub.rects[i];
                uint8_t*        pixels;
                int             pitch, j;

                if (!SDL_LockTexture(is->sub_texture, (SDL_Rect*)sub_rect, (void**)&pixels, &pitch))
                {
                  for (j = 0; j < sub_rect->h; j++, pixels += pitch)
                    memset(pixels, 0, sub_rect->w << 2);
                  SDL_UnlockTexture(is->sub_texture);
                }
              }
            }
            frame_queue_next(&is->subpq);
          }
          else
          {
            break;
          }
        }
      }

      frame_queue_next(&is->pictq);
      is->force_refresh = 1;

      if (is->step && !is->paused)
        stream_toggle_pause(is);
    }
  display:
    if (0) {};
    /* display picture */
    /* if (!display_disable && is->force_refresh && is->show_mode == SHOW_MODE_VIDEO && is->pictq.rindex_shown) */
    /*   video_display(is, index); */
  }
  is->force_refresh = 0;
  if (show_status)
  {
    AVBPrint       buf;
    static int64_t last_time;
    int64_t        cur_time;
    int            aqsize, vqsize, sqsize;
    double         av_diff;

    cur_time = av_gettime_relative();
    if (!last_time || (cur_time - last_time) >= 30000)
    {
      aqsize = 0;
      vqsize = 0;
      sqsize = 0;
      if (is->audio_st) aqsize = is->audioq.size;
      if (is->video_st) vqsize = is->videoq.size;
      if (is->subtitle_st) sqsize = is->subtitleq.size;
      av_diff = 0;
      if (is->audio_st && is->video_st)
        av_diff = get_clock(&is->audclk) - get_clock(&is->vidclk);
      else if (is->video_st)
        av_diff = get_master_clock(is) - get_clock(&is->vidclk);
      else if (is->audio_st)
        av_diff = get_master_clock(is) - get_clock(&is->audclk);

      /* av_bprint_init(&buf, 0, AV_BPRINT_SIZE_AUTOMATIC); */
      /* av_bprintf(&buf, */
      /*            "%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%" PRId64 "/%" PRId64 "   \r", */
      /*            get_master_clock(is), */
      /*            (is->audio_st && is->video_st) ? "A-V" : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")), */
      /*            av_diff, */
      /*            is->frame_drops_early + is->frame_drops_late, */
      /*            aqsize / 1024, */
      /*            vqsize / 1024, */
      /*            sqsize, */
      /*            is->video_st ? is->viddec.avctx->pts_correction_num_faulty_dts : 0, */
      /*            is->video_st ? is->viddec.avctx->pts_correction_num_faulty_pts : 0); */

      /* if (show_status == 1 && AV_LOG_INFO > av_log_get_level()) */
      /*   fprintf(stderr, "%s", buf.str); */
      /* else */
      /*   av_log(NULL, AV_LOG_INFO, "%s", buf.str); */

      /* fflush(stderr); */
      /* av_bprint_finalize(&buf, NULL); */

      last_time = cur_time;
    }
  }
}

#endif

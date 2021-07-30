#ifndef stream_h
#define stream_h

#include "libavformat/avformat.h"
#include "video.c"

VideoState* stream_open(const char* filename, AVInputFormat* iformat);
void        stream_close(VideoState* is);
void        stream_seek(VideoState* is, int64_t pos, int64_t rel, int seek_by_bytes);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "codec.c"
#include "decoder.c"
#include "libavdevice/avdevice.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavformat/avformat.h"
#include "libavutil/avstring.h"
#include "libavutil/display.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "options.c"
#include "packet.c"
#include "strcomm.c"
#include "video.c"
#include <SDL.h>
#include <stdint.h>

const char*       audio_codec_name;
const char*       subtitle_codec_name;
const char*       video_codec_name;
SDL_AudioDeviceID audio_dev;

/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB 20

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

int decoder_reorder_pts = -1;

AVDictionary* sws_dict;

const char** vfilters_list = NULL;
int          nb_vfilters   = 0;

static SDL_RendererInfo renderer_info = {0};

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

static void set_default_window_size(int width, int height, AVRational sar)
{
  SDL_Rect rect;
  int      max_width  = screen_width ? screen_width : INT_MAX;
  int      max_height = screen_height ? screen_height : INT_MAX;
  if (max_width == INT_MAX && max_height == INT_MAX)
    max_height = height;
  calculate_display_rect(&rect, 0, 0, max_width, max_height, width, height, sar);
  default_width  = rect.w;
  default_height = rect.h;
}

static int queue_picture(VideoState* is, AVFrame* src_frame, double pts, double duration, int64_t pos, int serial)
{
  Frame* vp;

#if defined(DEBUG_SYNC)
  printf("frame_type=%c pts=%0.3f\n",
         av_get_picture_type_char(src_frame->pict_type), pts);
#endif

  if (!(vp = frame_queue_peek_writable(&is->pictq)))
    return -1;

  vp->sar      = src_frame->sample_aspect_ratio;
  vp->uploaded = 0;

  vp->width  = src_frame->width;
  vp->height = src_frame->height;
  vp->format = src_frame->format;

  vp->pts      = pts;
  vp->duration = duration;
  vp->pos      = pos;
  vp->serial   = serial;

  set_default_window_size(vp->width, vp->height, vp->sar);

  av_frame_move_ref(vp->frame, src_frame);
  frame_queue_push(&is->pictq);
  return 0;
}

/* copy samples for viewing in editor window */
void update_sample_display(VideoState* is, short* samples, int samples_size)
{
  int size, len;

  size = samples_size / sizeof(short);
  while (size > 0)
  {
    len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
    if (len > size)
      len = size;
    memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
    samples += len;
    is->sample_array_index += len;
    if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
      is->sample_array_index = 0;
    size -= len;
  }
}

void stream_component_close(VideoState* is, int stream_index)
{
  AVFormatContext*   ic = is->ic;
  AVCodecParameters* codecpar;

  if (stream_index < 0 || stream_index >= ic->nb_streams)
    return;
  codecpar = ic->streams[stream_index]->codecpar;

  switch (codecpar->codec_type)
  {
  case AVMEDIA_TYPE_AUDIO:
    decoder_abort(&is->auddec, &is->sampq);
    SDL_CloseAudioDevice(audio_dev);
    decoder_destroy(&is->auddec);
    swr_free(&is->swr_ctx);
    av_freep(&is->audio_buf1);
    is->audio_buf1_size = 0;
    is->audio_buf       = NULL;

    if (is->rdft)
    {
      av_rdft_end(is->rdft);
      av_freep(&is->rdft_data);
      is->rdft      = NULL;
      is->rdft_bits = 0;
    }
    break;
  case AVMEDIA_TYPE_VIDEO:
    decoder_abort(&is->viddec, &is->pictq);
    decoder_destroy(&is->viddec);
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    decoder_abort(&is->subdec, &is->subpq);
    decoder_destroy(&is->subdec);
    break;
  default:
    break;
  }

  ic->streams[stream_index]->discard = AVDISCARD_ALL;
  switch (codecpar->codec_type)
  {
  case AVMEDIA_TYPE_AUDIO:
    is->audio_st     = NULL;
    is->audio_stream = -1;
    break;
  case AVMEDIA_TYPE_VIDEO:
    is->video_st     = NULL;
    is->video_stream = -1;
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    is->subtitle_st     = NULL;
    is->subtitle_stream = -1;
    break;
  default:
    break;
  }
}

void stream_close(VideoState* is)
{
  /* XXX: use a special url_shutdown call to abort parse cleanly */
  is->abort_request = 1;
  SDL_WaitThread(is->read_tid, NULL);

  /* close each stream */
  if (is->audio_stream >= 0)
    stream_component_close(is, is->audio_stream);
  if (is->video_stream >= 0)
    stream_component_close(is, is->video_stream);
  if (is->subtitle_stream >= 0)
    stream_component_close(is, is->subtitle_stream);

  avformat_close_input(&is->ic);

  packet_queue_destroy(&is->videoq);
  packet_queue_destroy(&is->audioq);
  packet_queue_destroy(&is->subtitleq);

  /* free all pictures */
  frame_queue_destory(&is->pictq);
  frame_queue_destory(&is->sampq);
  frame_queue_destory(&is->subpq);
  SDL_DestroyCond(is->continue_read_thread);
  sws_freeContext(is->img_convert_ctx);
  sws_freeContext(is->sub_convert_ctx);
  av_free(is->filename);
  if (is->vis_texture)
    SDL_DestroyTexture(is->vis_texture);
  if (is->vid_texture)
    SDL_DestroyTexture(is->vid_texture);
  if (is->sub_texture)
    SDL_DestroyTexture(is->sub_texture);
  av_free(is);
}

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
int synchronize_audio(VideoState* is, int nb_samples)
{
  int wanted_nb_samples = nb_samples;

  /* if not master, then we try to remove or add samples to correct the clock */
  if (get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER)
  {
    double diff, avg_diff;
    int    min_nb_samples, max_nb_samples;

    diff = get_clock(&is->audclk) - get_master_clock(is);

    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
    {
      is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
      if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
      {
        /* not enough measures to have a correct estimate */
        is->audio_diff_avg_count++;
      }
      else
      {
        /* estimate the A-V difference */
        avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

        if (fabs(avg_diff) >= is->audio_diff_threshold)
        {
          wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
          min_nb_samples    = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
          max_nb_samples    = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
          wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
        }
        av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n",
               diff, avg_diff, wanted_nb_samples - nb_samples,
               is->audio_clock, is->audio_diff_threshold);
      }
    }
    else
    {
      /* too big difference : may be initial PTS errors, so
               reset A-V filter */
      is->audio_diff_avg_count = 0;
      is->audio_diff_cum       = 0;
    }
  }

  return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
int audio_decode_frame(VideoState* is)
{
  int              data_size, resampled_data_size;
  int64_t          dec_channel_layout;
  av_unused double audio_clock0;
  int              wanted_nb_samples;
  Frame*           af;

  if (is->paused)
    return -1;

  do
  {
#if defined(_WIN32)
    while (frame_queue_nb_remaining(&is->sampq) == 0)
    {
      if ((av_gettime_relative() - audio_callback_time) > 1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
        return -1;
      av_usleep(1000);
    }
#endif
    if (!(af = frame_queue_peek_readable(&is->sampq)))
      return -1;
    frame_queue_next(&is->sampq);
  } while (af->serial != is->audioq.serial);

  data_size = av_samples_get_buffer_size(NULL, af->frame->channels,
                                         af->frame->nb_samples,
                                         af->frame->format, 1);

  dec_channel_layout =
      (af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ? af->frame->channel_layout : av_get_default_channel_layout(af->frame->channels);
  wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);

  if (af->frame->format != is->audio_src.fmt ||
      dec_channel_layout != is->audio_src.channel_layout ||
      af->frame->sample_rate != is->audio_src.freq ||
      (wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx))
  {
    swr_free(&is->swr_ctx);
    is->swr_ctx = swr_alloc_set_opts(NULL,
                                     is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
                                     dec_channel_layout, af->frame->format, af->frame->sample_rate,
                                     0, NULL);
    if (!is->swr_ctx || swr_init(is->swr_ctx) < 0)
    {
      av_log(NULL, AV_LOG_ERROR,
             "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
             af->frame->sample_rate, av_get_sample_fmt_name(af->frame->format), af->frame->channels,
             is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
      swr_free(&is->swr_ctx);
      return -1;
    }
    is->audio_src.channel_layout = dec_channel_layout;
    is->audio_src.channels       = af->frame->channels;
    is->audio_src.freq           = af->frame->sample_rate;
    is->audio_src.fmt            = af->frame->format;
  }

  if (is->swr_ctx)
  {
    const uint8_t** in        = (const uint8_t**)af->frame->extended_data;
    uint8_t**       out       = &is->audio_buf1;
    int             out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
    int             out_size  = av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
    int             len2;
    if (out_size < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
      return -1;
    }
    if (wanted_nb_samples != af->frame->nb_samples)
    {
      if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate,
                               wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0)
      {
        av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
        return -1;
      }
    }
    av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
    if (!is->audio_buf1)
      return AVERROR(ENOMEM);
    len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
    if (len2 < 0)
    {
      av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
      return -1;
    }
    if (len2 == out_count)
    {
      av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
      if (swr_init(is->swr_ctx) < 0)
        swr_free(&is->swr_ctx);
    }
    is->audio_buf       = is->audio_buf1;
    resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
  }
  else
  {
    is->audio_buf       = af->frame->data[0];
    resampled_data_size = data_size;
  }

  audio_clock0 = is->audio_clock;
  /* update the audio clock with the pts */
  if (!isnan(af->pts))
    is->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
  else
    is->audio_clock = NAN;
  is->audio_clock_serial = af->serial;
#ifdef DEBUG
  {
    double last_clock;
    /* printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n", */
    /*        is->audio_clock - last_clock, */
    /*        is->audio_clock, audio_clock0); */
    last_clock = is->audio_clock;
  }
#endif
  return resampled_data_size;
}

/* prepare a new audio buffer */
void sdl_audio_callback(void* opaque, Uint8* stream, int len)
{
  VideoState* is = opaque;
  int         audio_size, len1;

  audio_callback_time = av_gettime_relative();

  while (len > 0)
  {
    if (is->audio_buf_index >= is->audio_buf_size)
    {
      audio_size = audio_decode_frame(is);
      if (audio_size < 0)
      {
        /* if error, just output silence */
        is->audio_buf      = NULL;
        is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
      }
      else
      {
        if (is->show_mode != SHOW_MODE_VIDEO)
          update_sample_display(is, (int16_t*)is->audio_buf, audio_size);
        is->audio_buf_size = audio_size;
      }
      is->audio_buf_index = 0;
    }
    len1 = is->audio_buf_size - is->audio_buf_index;
    if (len1 > len)
      len1 = len;
    if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME)
      memcpy(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, len1);
    else
    {
      memset(stream, 0, len1);
      if (!is->muted && is->audio_buf)
        SDL_MixAudioFormat(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, AUDIO_S16SYS, len1, is->audio_volume);
    }
    len -= len1;
    stream += len1;
    is->audio_buf_index += len1;
  }
  is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
  /* Let's assume the audio driver that is used by SDL has two periods. */
  if (!isnan(is->audio_clock))
  {
    set_clock_at(&is->audclk, is->audio_clock - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec, is->audio_clock_serial, audio_callback_time / 1000000.0);
    sync_clock_to_slave(&is->extclk, &is->audclk);
  }
}

int audio_open(void* opaque, int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, struct AudioParams* audio_hw_params)
{
  SDL_AudioSpec    wanted_spec, spec;
  const char*      env;
  static const int next_nb_channels[]   = {0, 0, 1, 6, 2, 6, 4, 6};
  static const int next_sample_rates[]  = {0, 44100, 48000, 96000, 192000};
  int              next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

  env = SDL_getenv("SDL_AUDIO_CHANNELS");
  if (env)
  {
    wanted_nb_channels    = atoi(env);
    wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
  }
  if (!wanted_channel_layout || wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout))
  {
    wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
    wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
  }
  wanted_nb_channels   = av_get_channel_layout_nb_channels(wanted_channel_layout);
  wanted_spec.channels = wanted_nb_channels;
  wanted_spec.freq     = wanted_sample_rate;
  if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0)
  {
    av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
    return -1;
  }
  while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
    next_sample_rate_idx--;
  wanted_spec.format   = AUDIO_S16SYS;
  wanted_spec.silence  = 0;
  wanted_spec.samples  = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
  wanted_spec.callback = sdl_audio_callback;
  wanted_spec.userdata = opaque;
  while (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE)))
  {
    av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n",
           wanted_spec.channels, wanted_spec.freq, SDL_GetError());
    wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
    if (!wanted_spec.channels)
    {
      wanted_spec.freq     = next_sample_rates[next_sample_rate_idx--];
      wanted_spec.channels = wanted_nb_channels;
      if (!wanted_spec.freq)
      {
        av_log(NULL, AV_LOG_ERROR,
               "No more combinations to try, audio open failed\n");
        return -1;
      }
    }
    wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
  }
  if (spec.format != AUDIO_S16SYS)
  {
    av_log(NULL, AV_LOG_ERROR,
           "SDL advised audio format %d is not supported!\n", spec.format);
    return -1;
  }
  if (spec.channels != wanted_spec.channels)
  {
    wanted_channel_layout = av_get_default_channel_layout(spec.channels);
    if (!wanted_channel_layout)
    {
      av_log(NULL, AV_LOG_ERROR,
             "SDL advised channel count %d is not supported!\n", spec.channels);
      return -1;
    }
  }

  audio_hw_params->fmt            = AV_SAMPLE_FMT_S16;
  audio_hw_params->freq           = spec.freq;
  audio_hw_params->channel_layout = wanted_channel_layout;
  audio_hw_params->channels       = spec.channels;
  audio_hw_params->frame_size     = av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
  audio_hw_params->bytes_per_sec  = av_samples_get_buffer_size(NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
  if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0)
  {
    av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
    return -1;
  }
  return spec.size;
}

enum ShowMode show_mode = SHOW_MODE_NONE;

#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

#define MIN_FRAMES 25

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

static int get_video_frame(VideoState* is, AVFrame* frame)
{
  int got_picture;

  if ((got_picture = decoder_decode_frame(&is->viddec, frame, NULL)) < 0)
    return -1;

  if (got_picture)
  {
    double dpts = NAN;

    if (frame->pts != AV_NOPTS_VALUE)
      dpts = av_q2d(is->video_st->time_base) * frame->pts;

    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

    if (framedrop > 0 || (framedrop && get_master_sync_type(is) != AV_SYNC_VIDEO_MASTER))
    {
      if (frame->pts != AV_NOPTS_VALUE)
      {
        double diff = dpts - get_master_clock(is);
        if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
            diff - is->frame_last_filter_delay < 0 &&
            is->viddec.pkt_serial == is->vidclk.serial &&
            is->videoq.nb_packets)
        {
          is->frame_drops_early++;
          av_frame_unref(frame);
          got_picture = 0;
        }
      }
    }
  }

  return got_picture;
}

double get_rotation(AVStream* st)
{
  uint8_t* displaymatrix = av_stream_get_side_data(st,
                                                   AV_PKT_DATA_DISPLAYMATRIX, NULL);
  double   theta         = 0;
  if (displaymatrix)
    theta = -av_display_rotation_get((int32_t*)displaymatrix);

  theta -= 360 * floor(theta / 360 + 0.9 / 360);

  if (fabs(theta - 90 * round(theta / 90)) > 2)
    av_log(NULL, AV_LOG_WARNING, "Odd rotation angle.\n"
                                 "If you want to help, upload a sample "
                                 "of this file to https://streams.videolan.org/upload/ "
                                 "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");

  return theta;
}

int cmp_audio_fmts(enum AVSampleFormat fmt1, int64_t channel_count1, enum AVSampleFormat fmt2, int64_t channel_count2)
{
  /* If channel count == 1, planar and non-planar formats are the same */
  if (channel_count1 == 1 && channel_count2 == 1)
    return av_get_packed_sample_fmt(fmt1) != av_get_packed_sample_fmt(fmt2);
  else
    return channel_count1 != channel_count2 || fmt1 != fmt2;
}

int64_t get_valid_channel_layout(int64_t channel_layout, int channels)
{
  if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
    return channel_layout;
  else
    return 0;
}

int is_realtime(AVFormatContext* s)
{
  if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") || !strcmp(s->iformat->name, "sdp"))
    return 1;

  if (s->pb && (!strncmp(s->url, "rtp:", 4) || !strncmp(s->url, "udp:", 4)))
    return 1;
  return 0;
}

int stream_has_enough_packets(AVStream* st, int stream_id, PacketQueue* queue)
{
  return stream_id < 0 ||
         queue->abort_request ||
         (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
         queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}

/* seek in the stream */
void stream_seek(VideoState* is, int64_t pos, int64_t rel, int seek_by_bytes)
{
  if (!is->seek_req)
  {
    is->seek_pos = pos;
    is->seek_rel = rel;
    is->seek_flags &= ~AVSEEK_FLAG_BYTE;
    if (seek_by_bytes)
      is->seek_flags |= AVSEEK_FLAG_BYTE;
    is->seek_req = 1;
    SDL_CondSignal(is->continue_read_thread);
  }
}

void step_to_next_frame(VideoState* is)
{
  /* if the stream is paused unpause it, then step */
  if (is->paused)
    stream_toggle_pause(is);
  is->step = 1;
}

AVDictionary** setup_find_stream_info_opts(AVFormatContext* s,
                                           AVDictionary*    codec_opts)
{
  int            i;
  AVDictionary** opts;

  if (!s->nb_streams)
    return NULL;
  opts = av_mallocz_array(s->nb_streams, sizeof(*opts));
  if (!opts)
  {
    av_log(NULL, AV_LOG_ERROR,
           "Could not alloc memory for stream options.\n");
    return NULL;
  }
  for (i = 0; i < s->nb_streams; i++)
    opts[i] = filter_codec_opts(codec_opts, s->streams[i]->codecpar->codec_id,
                                s, s->streams[i], NULL);
  return opts;
}

void print_error(const char* filename, int err)
{
  char        errbuf[128];
  const char* errbuf_ptr = errbuf;

  if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
    errbuf_ptr = strerror(AVUNERROR(err));
  av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}

int decode_interrupt_cb(void* ctx)
{
  VideoState* is = ctx;
  return is->abort_request;
}

static int audio_thread(void* arg)
{
  VideoState* is    = arg;
  AVFrame*    frame = av_frame_alloc();
  Frame*      af;
  /* #if CONFIG_AVFILTER */
  /*   int   v  last_serial = -1; */
  /*   int64_t dec_channel_layout; */
  /*   int     reconfigure; */
  /* #endif */
  int        got_frame = 0;
  AVRational tb;
  int        ret = 0;

  if (!frame)
    return AVERROR(ENOMEM);

  do
  {
    if ((got_frame = decoder_decode_frame(&is->auddec, frame, NULL)) < 0)
      goto the_end;

    if (got_frame)
    {
      tb = (AVRational){1, frame->sample_rate};

      /* #if CONFIG_AVFILTER */
      /*       dec_channel_layout = get_valid_channel_layout(frame->channel_layout, frame->channels); */

      /*       reconfigure = */
      /*           cmp_audio_fmts(is->audio_filter_src.fmt, is->audio_filter_src.channels, */
      /*                          frame->format, frame->channels) || */
      /*           is->audio_filter_src.channel_layout != dec_channel_layout || */
      /*           is->audio_filter_src.freq != frame->sample_rate || */
      /*           is->auddec.pkt_serial != last_serial; */

      /*       if (reconfigure) */
      /*       { */
      /*         char buf1[1024], buf2[1024]; */
      /*         av_get_channel_layout_string(buf1, sizeof(buf1), -1, is->audio_filter_src.channel_layout); */
      /*         av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout); */
      /*         av_log(NULL, AV_LOG_DEBUG, */
      /*                "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d fmt:%s layout:%s serial:%d\n", */
      /*                is->audio_filter_src.freq, is->audio_filter_src.channels, av_get_sample_fmt_name(is->audio_filter_src.fmt), buf1, last_serial, */
      /*                frame->sample_rate, frame->channels, av_get_sample_fmt_name(frame->format), buf2, is->auddec.pkt_serial); */

      /*         is->audio_filter_src.fmt            = frame->format; */
      /*         is->audio_filter_src.channels       = frame->channels; */
      /*         is->audio_filter_src.channel_layout = dec_channel_layout; */
      /*         is->audio_filter_src.freq           = frame->sample_rate; */
      /*         last_serial                         = is->auddec.pkt_serial; */

      /*         if ((ret = configure_audio_filters(is, afilters, 1)) < 0) */
      /*           goto the_end; */
      /*       } */

      /*       if ((ret = av_buffersrc_add_frame(is->in_audio_filter, frame)) < 0) */
      /*         goto the_end; */

      /*       while ((ret = av_buffersink_get_frame_flags(is->out_audio_filter, frame, 0)) >= 0) */
      /*       { */
      /*         tb = av_buffersink_get_time_base(is->out_audio_filter); */
      /* #endif */
      if (!(af = frame_queue_peek_writable(&is->sampq)))
        goto the_end;

      af->pts      = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
      af->pos      = frame->pkt_pos;
      af->serial   = is->auddec.pkt_serial;
      af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

      av_frame_move_ref(af->frame, frame);
      frame_queue_push(&is->sampq);

      /* #if CONFIG_AVFILTER */
      /*         if (is->audioq.serial != is->auddec.pkt_serial) */
      /*           break; */
      /*       } */
      /*       if (ret == AVERROR_EOF) */
      /*         is->auddec.finished = is->auddec.pkt_serial; */
      /* #endif */
    }
  } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
  /* #if CONFIG_AVFILTER */
  /*   avfilter_graph_free(&is->agraph); */
  /* #endif */
  av_frame_free(&frame);
  return ret;
}

static int video_thread(void* arg)
{
  VideoState* is    = arg;
  AVFrame*    frame = av_frame_alloc();
  double      pts;
  double      duration;
  int         ret;
  AVRational  tb         = is->video_st->time_base;
  AVRational  frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

  /* #if CONFIG_AVFILTER */
  /*     AVFilterGraph *graph = NULL; */
  /*     AVFilterContext *filt_out = NULL, *filt_in = NULL; */
  /*     int last_w = 0; */
  /*     int last_h = 0; */
  /*     enum AVPixelFormat last_format = -2; */
  /*     int last_serial = -1; */
  /*     int last_vfilter_idx = 0; */
  /* #endif */

  if (!frame)
    return AVERROR(ENOMEM);

  for (;;)
  {
    ret = get_video_frame(is, frame);
    if (ret < 0)
      goto the_end;
    if (!ret)
      continue;

    /* #if CONFIG_AVFILTER */
    /*         if (   last_w != frame->width */
    /*             || last_h != frame->height */
    /*             || last_format != frame->format */
    /*             || last_serial != is->viddec.pkt_serial */
    /*             || last_vfilter_idx != is->vfilter_idx) { */
    /*             av_log(NULL, AV_LOG_DEBUG, */
    /*                    "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s serial:%d\n", */
    /*                    last_w, last_h, */
    /*                    (const char *)av_x_if_null(av_get_pix_fmt_name(last_format), "none"), last_serial, */
    /*                    frame->width, frame->height, */
    /*                    (const char *)av_x_if_null(av_get_pix_fmt_name(frame->format), "none"), is->viddec.pkt_serial); */
    /*             avfilter_graph_free(&graph); */
    /*             graph = avfilter_graph_alloc(); */
    /*             if (!graph) { */
    /*                 ret = AVERROR(ENOMEM); */
    /*                 goto the_end; */
    /*             } */
    /*             graph->nb_threads = filter_nbthreads; */
    /*             if ((ret = configure_video_filters(graph, is, vfilters_list ? vfilters_list[is->vfilter_idx] : NULL, frame)) < 0) { */
    /*                 SDL_Event event; */
    /*                 event.type = FF_QUIT_EVENT; */
    /*                 event.user.data1 = is; */
    /*                 SDL_PushEvent(&event); */
    /*                 goto the_end; */
    /*             } */
    /*             filt_in  = is->in_video_filter; */
    /*             filt_out = is->out_video_filter; */
    /*             last_w = frame->width; */
    /*             last_h = frame->height; */
    /*             last_format = frame->format; */
    /*             last_serial = is->viddec.pkt_serial; */
    /*             last_vfilter_idx = is->vfilter_idx; */
    /*             frame_rate = av_buffersink_get_frame_rate(filt_out); */
    /*         } */

    /*         ret = av_buffersrc_add_frame(filt_in, frame); */
    /*         if (ret < 0) */
    /*             goto the_end; */

    /*         while (ret >= 0) { */
    /*             is->frame_last_returned_time = av_gettime_relative() / 1000000.0; */

    /*             ret = av_buffersink_get_frame_flags(filt_out, frame, 0); */
    /*             if (ret < 0) { */
    /*                 if (ret == AVERROR_EOF) */
    /*                     is->viddec.finished = is->viddec.pkt_serial; */
    /*                 ret = 0; */
    /*                 break; */
    /*             } */

    /*             is->frame_last_filter_delay = av_gettime_relative() / 1000000.0 - is->frame_last_returned_time; */
    /*             if (fabs(is->frame_last_filter_delay) > AV_NOSYNC_THRESHOLD / 10.0) */
    /*                 is->frame_last_filter_delay = 0; */
    /*             tb = av_buffersink_get_time_base(filt_out); */
    /* #endif */
    duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
    pts      = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
    ret      = queue_picture(is, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial);
    av_frame_unref(frame);
    /* #if CONFIG_AVFILTER */
    /*             if (is->videoq.serial != is->viddec.pkt_serial) */
    /*                 break; */
    /*         } */
    /* #endif */

    if (ret < 0)
      goto the_end;
  }
the_end:
  /* #if CONFIG_AVFILTER */
  /*     avfilter_graph_free(&graph); */
  /* #endif */
  av_frame_free(&frame);
  return 0;
}

int subtitle_thread(void* arg)
{
  VideoState* is = arg;
  Frame*      sp;
  int         got_subtitle;
  double      pts;

  for (;;)
  {
    if (!(sp = frame_queue_peek_writable(&is->subpq)))
      return 0;

    if ((got_subtitle = decoder_decode_frame(&is->subdec, NULL, &sp->sub)) < 0)
      break;

    pts = 0;

    if (got_subtitle && sp->sub.format == 0)
    {
      if (sp->sub.pts != AV_NOPTS_VALUE)
        pts = sp->sub.pts / (double)AV_TIME_BASE;
      sp->pts      = pts;
      sp->serial   = is->subdec.pkt_serial;
      sp->width    = is->subdec.avctx->width;
      sp->height   = is->subdec.avctx->height;
      sp->uploaded = 0;

      /* now we can update the picture count */
      frame_queue_push(&is->subpq);
    }
    else if (got_subtitle)
    {
      avsubtitle_free(&sp->sub);
    }
  }
  return 0;
}

/* open a given stream. Return 0 if OK */
int stream_component_open(VideoState* is, int stream_index)
{
  AVFormatContext*   ic = is->ic;
  AVCodecContext*    avctx;
  AVCodec*           codec;
  const char*        forced_codec_name = NULL;
  AVDictionary*      opts              = NULL;
  AVDictionaryEntry* t                 = NULL;
  int                sample_rate, nb_channels;
  int64_t            channel_layout;
  int                ret           = 0;
  int                stream_lowres = lowres;

  if (stream_index < 0 || stream_index >= ic->nb_streams)
    return -1;

  avctx = avcodec_alloc_context3(NULL);
  if (!avctx)
    return AVERROR(ENOMEM);

  ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
  if (ret < 0)
    goto fail;
  avctx->pkt_timebase = ic->streams[stream_index]->time_base;

  codec = avcodec_find_decoder(avctx->codec_id);

  switch (avctx->codec_type)
  {
  case AVMEDIA_TYPE_AUDIO:
    is->last_audio_stream = stream_index;
    forced_codec_name     = audio_codec_name;
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    is->last_subtitle_stream = stream_index;
    forced_codec_name        = subtitle_codec_name;
    break;
  case AVMEDIA_TYPE_VIDEO:
    is->last_video_stream = stream_index;
    forced_codec_name     = video_codec_name;
    break;
  default:
    break;
  }
  if (forced_codec_name)
    codec = avcodec_find_decoder_by_name(forced_codec_name);
  if (!codec)
  {
    if (forced_codec_name)
      av_log(NULL, AV_LOG_WARNING,
             "No codec could be found with name '%s'\n", forced_codec_name);
    else
      av_log(NULL, AV_LOG_WARNING,
             "No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
    ret = AVERROR(EINVAL);
    goto fail;
  }

  avctx->codec_id = codec->id;
  if (stream_lowres > codec->max_lowres)
  {
    av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
           codec->max_lowres);
    stream_lowres = codec->max_lowres;
  }
  avctx->lowres = stream_lowres;

  if (fast)
    avctx->flags2 |= AV_CODEC_FLAG2_FAST;

  opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
  if (!av_dict_get(opts, "threads", NULL, 0))
    av_dict_set(&opts, "threads", "auto", 0);
  if (stream_lowres)
    av_dict_set_int(&opts, "lowres", stream_lowres, 0);
  if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
    av_dict_set(&opts, "refcounted_frames", "1", 0);
  if ((ret = avcodec_open2(avctx, codec, &opts)) < 0)
  {
    goto fail;
  }
  if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX)))
  {
    av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
    ret = AVERROR_OPTION_NOT_FOUND;
    goto fail;
  }

  is->eof                            = 0;
  ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
  switch (avctx->codec_type)
  {
  case AVMEDIA_TYPE_AUDIO:
    /* #if CONFIG_AVFILTER */
    /*         { */
    /*             AVFilterContext *sink; */

    /*             is->audio_filter_src.freq           = avctx->sample_rate; */
    /*             is->audio_filter_src.channels       = avctx->channels; */
    /*             is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels); */
    /*             is->audio_filter_src.fmt            = avctx->sample_fmt; */
    /*             if ((ret = configure_audio_filters(is, afilters, 0)) < 0) */
    /*                 goto fail; */
    /*             sink = is->out_audio_filter; */
    /*             sample_rate    = av_buffersink_get_sample_rate(sink); */
    /*             nb_channels    = av_buffersink_get_channels(sink); */
    /*             channel_layout = av_buffersink_get_channel_layout(sink); */
    /*         } */
    /* #else */
    sample_rate    = avctx->sample_rate;
    nb_channels    = avctx->channels;
    channel_layout = avctx->channel_layout;
    /*#endif*/

    /* prepare audio output */
    if ((ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
      goto fail;
    is->audio_hw_buf_size = ret;
    is->audio_src         = is->audio_tgt;
    is->audio_buf_size    = 0;
    is->audio_buf_index   = 0;

    /* init averaging filter */
    is->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    is->audio_diff_avg_count = 0;
    /* since we do not have a precise anough audio FIFO fullness,
           we correct audio sync only if larger than this threshold */
    is->audio_diff_threshold = (double)(is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

    is->audio_stream = stream_index;
    is->audio_st     = ic->streams[stream_index];

    decoder_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
    if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !is->ic->iformat->read_seek)
    {
      is->auddec.start_pts    = is->audio_st->start_time;
      is->auddec.start_pts_tb = is->audio_st->time_base;
    }
    if ((ret = decoder_start(&is->auddec, audio_thread, "audio_decoder", is)) < 0)
      goto out;
    SDL_PauseAudioDevice(audio_dev, 0);
    break;
  case AVMEDIA_TYPE_VIDEO:
    is->video_stream = stream_index;
    is->video_st     = ic->streams[stream_index];

    decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
    if ((ret = decoder_start(&is->viddec, video_thread, "video_decoder", is)) < 0)
      goto out;
    is->queue_attachments_req = 1;
    break;
  case AVMEDIA_TYPE_SUBTITLE:
    is->subtitle_stream = stream_index;
    is->subtitle_st     = ic->streams[stream_index];

    decoder_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
    if ((ret = decoder_start(&is->subdec, subtitle_thread, "subtitle_decoder", is)) < 0)
      goto out;
    break;
  default:
    break;
  }
  goto out;

fail:
  avcodec_free_context(&avctx);
out:
  av_dict_free(&opts);

  return ret;
}

/* this thread gets the stream from the disk or the network */
int read_thread(void* arg)
{
  VideoState*        is = arg;
  AVFormatContext*   ic = NULL;
  int                err, i, ret;
  int                st_index[AVMEDIA_TYPE_NB];
  AVPacket           pkt1, *pkt = &pkt1;
  int64_t            stream_start_time;
  int                pkt_in_play_range = 0;
  AVDictionaryEntry* t;
  SDL_mutex*         wait_mutex        = SDL_CreateMutex();
  int                scan_all_pmts_set = 0;
  int64_t            pkt_ts;

  if (!wait_mutex)
  {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
    ret = AVERROR(ENOMEM);
    goto fail;
  }

  memset(st_index, -1, sizeof(st_index));
  is->eof = 0;

  ic = avformat_alloc_context();
  if (!ic)
  {
    av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
    ret = AVERROR(ENOMEM);
    goto fail;
  }
  ic->interrupt_callback.callback = decode_interrupt_cb;
  ic->interrupt_callback.opaque   = is;
  if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE))
  {
    av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    scan_all_pmts_set = 1;
  }
  err = avformat_open_input(&ic, is->filename, is->iformat, &format_opts);
  if (err < 0)
  {
    print_error(is->filename, err);
    ret = -1;
    goto fail;
  }
  if (scan_all_pmts_set)
    av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

  if ((t = av_dict_get(format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX)))
  {
    av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
    ret = AVERROR_OPTION_NOT_FOUND;
    goto fail;
  }
  is->ic = ic;

  /* AVDictionaryEntry* tag = NULL; */
  /* while ((tag = av_dict_get(ic->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) */
  /*   printf("%s=%s\n", tag->key, tag->value); */

  if (genpts)
    ic->flags |= AVFMT_FLAG_GENPTS;

  av_format_inject_global_side_data(ic);

  if (find_stream_info)
  {
    AVDictionary** opts            = setup_find_stream_info_opts(ic, codec_opts);
    int            orig_nb_streams = ic->nb_streams;

    err = avformat_find_stream_info(ic, opts);

    for (i = 0; i < orig_nb_streams; i++)
      av_dict_free(&opts[i]);
    av_freep(&opts);

    if (err < 0)
    {
      av_log(NULL, AV_LOG_WARNING,
             "%s: could not find codec parameters\n", is->filename);
      ret = -1;
      goto fail;
    }
  }

  if (ic->pb)
    ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

  if (seek_by_bytes < 0)
    seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);

  is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

  is->duration = (float)ic->duration / (float)AV_TIME_BASE;

  //if (!window_title && (t = av_dict_get(ic->metadata, "title", NULL, 0)))
  //  window_title = av_asprintf("%s - %s", t->value, input_filename);

  /* if seeking requested, we execute it */
  if (start_time != AV_NOPTS_VALUE)
  {
    int64_t timestamp;

    timestamp = start_time;
    /* add the stream start time */
    if (ic->start_time != AV_NOPTS_VALUE)
      timestamp += ic->start_time;
    ret = avformat_seek_file(ic, -1, INT64_MIN, timestamp, INT64_MAX, 0);
    if (ret < 0)
    {
      av_log(NULL, AV_LOG_WARNING, "%s: could not seek to position %0.3f\n",
             is->filename, (double)timestamp / AV_TIME_BASE);
    }
  }

  is->realtime = is_realtime(ic);

  /* if (show_status) */
  /*   av_dump_format(ic, 0, is->filename, 0); */

  for (i = 0; i < ic->nb_streams; i++)
  {
    AVStream*        st   = ic->streams[i];
    enum AVMediaType type = st->codecpar->codec_type;
    st->discard           = AVDISCARD_ALL;
    if (type >= 0 && wanted_stream_spec[type] && st_index[type] == -1)
      if (avformat_match_stream_specifier(ic, st, wanted_stream_spec[type]) > 0)
        st_index[type] = i;
  }
  for (i = 0; i < AVMEDIA_TYPE_NB; i++)
  {
    if (wanted_stream_spec[i] && st_index[i] == -1)
    {
      av_log(NULL, AV_LOG_ERROR, "Stream specifier %s does not match any %s stream\n", wanted_stream_spec[i], av_get_media_type_string(i));
      st_index[i] = INT_MAX;
    }
  }

  if (!video_disable)
    st_index[AVMEDIA_TYPE_VIDEO] =
        av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                            st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
  if (!audio_disable)
    st_index[AVMEDIA_TYPE_AUDIO] =
        av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                            st_index[AVMEDIA_TYPE_AUDIO],
                            st_index[AVMEDIA_TYPE_VIDEO],
                            NULL, 0);
  if (!video_disable && !subtitle_disable)
    st_index[AVMEDIA_TYPE_SUBTITLE] =
        av_find_best_stream(ic, AVMEDIA_TYPE_SUBTITLE,
                            st_index[AVMEDIA_TYPE_SUBTITLE],
                            (st_index[AVMEDIA_TYPE_AUDIO] >= 0 ? st_index[AVMEDIA_TYPE_AUDIO] : st_index[AVMEDIA_TYPE_VIDEO]),
                            NULL, 0);

  is->show_mode = show_mode;
  if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
  {
    AVStream*          st       = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
    AVCodecParameters* codecpar = st->codecpar;
    AVRational         sar      = av_guess_sample_aspect_ratio(ic, st, NULL);
    if (codecpar->width)
      set_default_window_size(codecpar->width, codecpar->height, sar);
  }

  /* open the streams */
  if (st_index[AVMEDIA_TYPE_AUDIO] >= 0)
  {
    stream_component_open(is, st_index[AVMEDIA_TYPE_AUDIO]);
  }

  ret = -1;
  if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
  {
    ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
  }
  if (is->show_mode == SHOW_MODE_NONE)
    is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;

  if (st_index[AVMEDIA_TYPE_SUBTITLE] >= 0)
  {
    stream_component_open(is, st_index[AVMEDIA_TYPE_SUBTITLE]);
  }

  if (is->video_stream < 0 && is->audio_stream < 0)
  {
    av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
           is->filename);
    ret = -1;
    goto fail;
  }

  if (infinite_buffer < 0 && is->realtime)
    infinite_buffer = 1;

  for (;;)
  {
    if (is->abort_request)
      break;
    if (is->paused != is->last_paused)
    {
      is->last_paused = is->paused;
      if (is->paused)
        is->read_pause_return = av_read_pause(ic);
      else
        av_read_play(ic);
    }
#if CONFIG_RTSP_DEMUXER || CONFIG_MMSH_PROTOCOL
    if (is->paused &&
        (!strcmp(ic->iformat->name, "rtsp") ||
         (ic->pb && !strncmp(input_filename, "mmsh:", 5))))
    {
      /* wait 10 ms to avoid trying to get another packet */
      /* XXX: horrible */
      SDL_Delay(10);
      continue;
    }
#endif
    if (is->seek_req)
    {
      int64_t seek_target = is->seek_pos;
      int64_t seek_min    = is->seek_rel > 0 ? seek_target - is->seek_rel + 2 : INT64_MIN;
      int64_t seek_max    = is->seek_rel < 0 ? seek_target - is->seek_rel - 2 : INT64_MAX;
      // FIXME the +-2 is due to rounding being not done in the correct direction in generation
      //      of the seek_pos/seek_rel variables

      ret = avformat_seek_file(is->ic, -1, seek_min, seek_target, seek_max, is->seek_flags);
      if (ret < 0)
      {
        av_log(NULL, AV_LOG_ERROR,
               "%s: error while seeking\n", is->ic->url);
      }
      else
      {
        if (is->audio_stream >= 0)
        {
          packet_queue_flush(&is->audioq);
          packet_queue_put(&is->audioq, &flush_pkt);
        }
        if (is->subtitle_stream >= 0)
        {
          packet_queue_flush(&is->subtitleq);
          packet_queue_put(&is->subtitleq, &flush_pkt);
        }
        if (is->video_stream >= 0)
        {
          packet_queue_flush(&is->videoq);
          packet_queue_put(&is->videoq, &flush_pkt);
        }
        if (is->seek_flags & AVSEEK_FLAG_BYTE)
        {
          set_clock(&is->extclk, NAN, 0);
        }
        else
        {
          set_clock(&is->extclk, seek_target / (double)AV_TIME_BASE, 0);
        }
      }
      is->seek_req              = 0;
      is->queue_attachments_req = 1;
      is->eof                   = 0;
      if (is->paused)
        step_to_next_frame(is);
    }
    if (is->queue_attachments_req)
    {
      if (is->video_st && is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC)
      {
        AVPacket copy;
        if ((ret = av_packet_ref(&copy, &is->video_st->attached_pic)) < 0)
          goto fail;
        packet_queue_put(&is->videoq, &copy);
        packet_queue_put_nullpacket(&is->videoq, is->video_stream);
      }
      is->queue_attachments_req = 0;
    }

    /* if the queue are full, no need to read more */
    if (infinite_buffer < 1 &&
        (is->audioq.size + is->videoq.size + is->subtitleq.size > MAX_QUEUE_SIZE || (stream_has_enough_packets(is->audio_st, is->audio_stream, &is->audioq) &&
                                                                                     stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq) &&
                                                                                     stream_has_enough_packets(is->subtitle_st, is->subtitle_stream, &is->subtitleq))))
    {
      /* wait 10 ms */
      SDL_LockMutex(wait_mutex);
      SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
      SDL_UnlockMutex(wait_mutex);
      continue;
    }
    if (!is->paused &&
        (!is->audio_st || (is->auddec.finished == is->audioq.serial && frame_queue_nb_remaining(&is->sampq) == 0)) &&
        (!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0)))
    {
      printf("loop %i autoexit %i\n", loop, autoexit);
      /* if (loop != 1 && (!loop || --loop)) */
      /* { */
      /*   printf("seek to start\n"); */
      /*   stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0); */
      /* } */
      /* else if (autoexit) */
      /* { */
      is->play_finished = 1;
      printf("autoexit\n");
      ret = AVERROR_EOF;
      goto fail;
      /* } */
    }
    ret = av_read_frame(ic, pkt);
    if (ret < 0)
    {
      if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof)
      {
        if (is->video_stream >= 0)
          packet_queue_put_nullpacket(&is->videoq, is->video_stream);
        if (is->audio_stream >= 0)
          packet_queue_put_nullpacket(&is->audioq, is->audio_stream);
        if (is->subtitle_stream >= 0)
          packet_queue_put_nullpacket(&is->subtitleq, is->subtitle_stream);
        is->eof = 1;
      }
      if (ic->pb && ic->pb->error)
        break;
      SDL_LockMutex(wait_mutex);
      SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
      SDL_UnlockMutex(wait_mutex);
      continue;
    }
    else
    {
      is->eof = 0;
    }
    /* check if packet is in play range specified by user, then queue, otherwise discard */
    stream_start_time = ic->streams[pkt->stream_index]->start_time;
    pkt_ts            = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
    pkt_in_play_range = duration == AV_NOPTS_VALUE ||
                        (pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
                                    av_q2d(ic->streams[pkt->stream_index]->time_base) -
                                (double)(start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000 <=
                            ((double)duration / 1000000);
    if (pkt->stream_index == is->audio_stream && pkt_in_play_range)
    {
      packet_queue_put(&is->audioq, pkt);
    }
    else if (pkt->stream_index == is->video_stream && pkt_in_play_range && !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC))
    {
      packet_queue_put(&is->videoq, pkt);
    }
    else if (pkt->stream_index == is->subtitle_stream && pkt_in_play_range)
    {
      packet_queue_put(&is->subtitleq, pkt);
    }
    else
    {
      av_packet_unref(pkt);
    }
  }

  ret = 0;
fail:
  if (ic && !is->ic)
    avformat_close_input(&ic);

  if (ret != 0)
  {
    SDL_Event event;

    event.type       = FF_QUIT_EVENT;
    event.user.data1 = is;
    SDL_PushEvent(&event);
  }
  SDL_DestroyMutex(wait_mutex);
  return 0;
}

VideoState* stream_open(const char* filename, AVInputFormat* iformat)
{
  VideoState* is;

  is = av_mallocz(sizeof(VideoState));
  if (!is)
    return NULL;
  is->last_video_stream = is->video_stream = -1;
  is->last_audio_stream = is->audio_stream = -1;
  is->last_subtitle_stream = is->subtitle_stream = -1;
  is->filename                                   = av_strdup(filename);
  if (!is->filename) goto fail;
  is->iformat       = iformat;
  is->ytop          = 0;
  is->xleft         = 0;
  is->play_finished = 0;

  /* start video display */
  if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0) goto fail;
  if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0) goto fail;
  if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0) goto fail;

  if (packet_queue_init(&is->videoq) < 0 ||
      packet_queue_init(&is->audioq) < 0 ||
      packet_queue_init(&is->subtitleq) < 0)
    goto fail;

  if (!(is->continue_read_thread = SDL_CreateCond()))
  {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
    goto fail;
  }

  init_clock(&is->vidclk, &is->videoq.serial);
  init_clock(&is->audclk, &is->audioq.serial);
  init_clock(&is->extclk, &is->extclk.serial);
  is->audio_clock_serial = -1;
  if (startup_volume < 0) av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
  if (startup_volume > 100) av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);
  startup_volume   = av_clip(startup_volume, 0, 100);
  startup_volume   = av_clip(SDL_MIX_MAXVOLUME * startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
  is->audio_volume = startup_volume;
  is->muted        = 0;
  is->av_sync_type = av_sync_type;
  is->read_tid     = SDL_CreateThread(read_thread, "read_thread", is);
  if (!is->read_tid)
  {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
  fail:
    stream_close(is);
    return NULL;
  }
  return is;
}

#endif

#ifndef video_h
#define video_h

#include "clock.c"
#include "decoder.c"
#include "frame.c"
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include <SDL.h>
#include <stdint.h>

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

typedef struct AudioParams
{
  int                 freq;
  int                 channels;
  int64_t             channel_layout;
  enum AVSampleFormat fmt;
  int                 frame_size;
  int                 bytes_per_sec;
} AudioParams;

typedef struct VideoState
{
  SDL_Thread*      read_tid;
  AVInputFormat*   iformat;
  int              abort_request;
  int              force_refresh;
  int              paused;
  int              last_paused;
  int              queue_attachments_req;
  int              seek_req;
  int              seek_flags;
  int64_t          seek_pos;
  int64_t          seek_rel;
  int              read_pause_return;
  AVFormatContext* ic;
  int              realtime;
  int              duration;

  Clock audclk;
  Clock vidclk;
  Clock extclk;

  FrameQueue pictq;
  FrameQueue subpq;
  FrameQueue sampq;

  Decoder auddec;
  Decoder viddec;
  Decoder subdec;

  int audio_stream;

  int av_sync_type;

  double             audio_clock;
  int                audio_clock_serial;
  double             audio_diff_cum; /* used for AV difference average computation */
  double             audio_diff_avg_coef;
  double             audio_diff_threshold;
  int                audio_diff_avg_count;
  AVStream*          audio_st;
  PacketQueue        audioq;
  int                audio_hw_buf_size;
  uint8_t*           audio_buf;
  uint8_t*           audio_buf1;
  unsigned int       audio_buf_size; /* in bytes */
  unsigned int       audio_buf1_size;
  int                audio_buf_index; /* in bytes */
  int                audio_write_buf_size;
  int                audio_volume;
  int                muted;
  struct AudioParams audio_src;
  struct AudioParams audio_filter_src;
  struct AudioParams audio_tgt;
  struct SwrContext* swr_ctx;
  int                frame_drops_early;
  int                frame_drops_late;

  enum ShowMode
  {
    SHOW_MODE_NONE  = -1,
    SHOW_MODE_VIDEO = 0,
    SHOW_MODE_WAVES,
    SHOW_MODE_RDFT,
    SHOW_MODE_NB
  } show_mode;

  int16_t      sample_array[SAMPLE_ARRAY_SIZE];
  int          sample_array_index;
  int          last_i_start;
  RDFTContext* rdft;
  int          rdft_bits;
  FFTSample*   rdft_data;
  int          xpos;
  double       last_vis_time;
  SDL_Texture* vis_texture;
  SDL_Texture* sub_texture;
  SDL_Texture* vid_texture;

  int         subtitle_stream;
  AVStream*   subtitle_st;
  PacketQueue subtitleq;

  double             frame_timer;
  double             frame_last_returned_time;
  double             frame_last_filter_delay;
  int                video_stream;
  AVStream*          video_st;
  PacketQueue        videoq;
  double             max_frame_duration; // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
  struct SwsContext* img_convert_ctx;
  struct SwsContext* sub_convert_ctx;
  int                eof;
  int                play_finished;

  char* filename;
  int   width, height, xleft, ytop;
  int   step;

  int              vfilter_idx;
  AVFilterContext* in_video_filter;  // the first filter in the video chain
  AVFilterContext* out_video_filter; // the last filter in the video chain
  AVFilterContext* in_audio_filter;  // the first filter in the audio chain
  AVFilterContext* out_audio_filter; // the last filter in the audio chain
  AVFilterGraph*   agraph;           // audio filter graph

  int last_video_stream, last_audio_stream, last_subtitle_stream;

  SDL_cond* continue_read_thread;
} VideoState;

enum
{
  AV_SYNC_AUDIO_MASTER, /* default choice */
  AV_SYNC_VIDEO_MASTER,
  AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

int get_master_sync_type(VideoState* is);

double get_master_clock(VideoState* is);

#endif

#if __INCLUDE_LEVEL__ == 0

/* get the current master clock value */
double get_master_clock(VideoState* is)
{
  double val;

  switch (get_master_sync_type(is))
  {
  case AV_SYNC_VIDEO_MASTER:
    val = get_clock(&is->vidclk);
    break;
  case AV_SYNC_AUDIO_MASTER:
    val = get_clock(&is->audclk);
    break;
  default:
    val = get_clock(&is->extclk);
    break;
  }
  return val;
}

int get_master_sync_type(VideoState* is)
{
  if (is->av_sync_type == AV_SYNC_VIDEO_MASTER)
  {
    if (is->video_st)
      return AV_SYNC_VIDEO_MASTER;
    else
      return AV_SYNC_AUDIO_MASTER;
  }
  else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER)
  {
    if (is->audio_st)
      return AV_SYNC_AUDIO_MASTER;
    else
      return AV_SYNC_EXTERNAL_CLOCK;
  }
  else
  {
    return AV_SYNC_EXTERNAL_CLOCK;
  }
}

#endif

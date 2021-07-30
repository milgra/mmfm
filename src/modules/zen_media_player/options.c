#ifndef options_h
#define options_h

#include "libavformat/avformat.h"
#include <stdint.h>

extern int64_t audio_callback_time;

extern int startup_volume;
extern int av_sync_type;
extern int fast;
extern int lowres;

extern AVDictionary *format_opts, *codec_opts;
extern int           genpts;
extern int           find_stream_info;
extern int           seek_by_bytes;
extern int64_t       start_time;
extern const char*   wanted_stream_spec[AVMEDIA_TYPE_NB];
extern int           audio_disable;
extern int           video_disable;
extern int           subtitle_disable;
extern int           screen_width;
extern int           screen_height;
extern int           default_width;
extern int           default_height;
extern int           infinite_buffer;
extern int           loop;
extern int64_t       duration;
extern int           autoexit;
extern char*         afilters;
extern int           filter_nbthreads;
extern int           autorotate;

#endif

#if __INCLUDE_LEVEL__ == 0

#include "video.c"

int64_t audio_callback_time;

int startup_volume = 100;
int av_sync_type   = AV_SYNC_AUDIO_MASTER;
int fast           = 0;
int lowres         = 0;

AVDictionary *format_opts, *codec_opts;
int           genpts                              = 0;
int           find_stream_info                    = 1;
int           seek_by_bytes                       = -1;
int64_t       start_time                          = AV_NOPTS_VALUE;
const char*   wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
int           audio_disable;
int           video_disable;
int           subtitle_disable;
int           screen_width    = 0;
int           screen_height   = 0;
int           default_width   = 640;
int           default_height  = 480;
int           infinite_buffer = -1;
int           loop            = 1;
int64_t       duration        = AV_NOPTS_VALUE;
int           autoexit;
char*         afilters         = NULL;
int           filter_nbthreads = 0;
int           autorotate       = 1;

#endif

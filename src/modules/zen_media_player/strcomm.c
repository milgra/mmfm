#ifndef strcomm_h
#define strcomm_h

#include "video.c"

extern int framedrop;
extern int show_status;

void stream_toggle_pause(VideoState* is);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavutil/time.h"

int framedrop   = -1;
int show_status = -1;

/* pause or resume the video */
void stream_toggle_pause(VideoState* is)
{
  if (is->paused)
  {
    is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
    if (is->read_pause_return != AVERROR(ENOSYS))
    {
      is->vidclk.paused = 0;
    }
    set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
  }
  set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
  is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

#endif

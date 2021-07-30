#ifndef clock_h
#define clock_h

typedef struct Clock
{
  double pts;       /* clock base */
  double pts_drift; /* clock base minus time at which we updated the clock */
  double last_updated;
  double speed;
  int    serial; /* clock is based on a packet with this serial */
  int    paused;
  int*   queue_serial; /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

double get_clock(Clock* c);
void   set_clock_at(Clock* c, double pts, int serial, double time);
void   set_clock(Clock* c, double pts, int serial);
void   set_clock_speed(Clock* c, double speed);
void   init_clock(Clock* c, int* queue_serial);
void   sync_clock_to_slave(Clock* c, Clock* slave);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavutil/time.h"
#include <math.h>

/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

double get_clock(Clock* c)
{
  if (*c->queue_serial != c->serial)
    return NAN;
  if (c->paused)
  {
    return c->pts;
  }
  else
  {
    double time = av_gettime_relative() / 1000000.0;
    return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
  }
}

void set_clock_at(Clock* c, double pts, int serial, double time)
{
  c->pts          = pts;
  c->last_updated = time;
  c->pts_drift    = c->pts - time;
  c->serial       = serial;
}

void set_clock(Clock* c, double pts, int serial)
{
  double time = av_gettime_relative() / 1000000.0;
  set_clock_at(c, pts, serial, time);
}

void set_clock_speed(Clock* c, double speed)
{
  set_clock(c, get_clock(c), c->serial);
  c->speed = speed;
}

void init_clock(Clock* c, int* queue_serial)
{
  c->speed        = 1.0;
  c->paused       = 0;
  c->queue_serial = queue_serial;
  set_clock(c, NAN, -1);
}

void sync_clock_to_slave(Clock* c, Clock* slave)
{
  double clock       = get_clock(c);
  double slave_clock = get_clock(slave);
  if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
    set_clock(c, slave_clock, slave->serial);
}

#endif

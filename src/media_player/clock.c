#ifndef clock_h
#define clock_h

#include "libavutil/opt.h"

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

void   clock_init(Clock* c, int* queue_serial);
double clock_get(Clock* c);
void   clock_set(Clock* c, double pts, int serial);
void   clock_set_at(Clock* c, double pts, int serial, double time);
void   clock_set_speed(Clock* c, double speed);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavutil/time.h"
#include "mt_log.c"

void clock_init(Clock* c, int* queue_serial)
{
    c->speed        = 1.0;
    c->paused       = 0;
    c->queue_serial = queue_serial;
    clock_set(c, NAN, -1);
}

double clock_get(Clock* c)
{
    if (*c->queue_serial != c->serial) return NAN;

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

void clock_set(Clock* c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    clock_set_at(c, pts, serial, time);
}

void clock_set_at(Clock* c, double pts, int serial, double time)
{
    c->pts          = pts;
    c->last_updated = time;
    c->pts_drift    = c->pts - time;
    c->serial       = serial;

    // mt_log_debug("clock set at pts %f last upd %f drift %f", c->pts, c->last_updated, c->pts_drift);
}

void clock_set_speed(Clock* c, double speed)
{
    clock_set(c, clock_get(c), c->serial);
    c->speed = speed;
}

#endif

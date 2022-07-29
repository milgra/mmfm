#ifndef viewer_h
#define viewer_h

void viewer_open(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavutil/fifo.h"
#include "libavutil/frame.h"
#include "zc_log.c"
#include <SDL.h>
#include <SDL_thread.h>

#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE 16

typedef struct PacketQueue
{
    AVFifo*    pkt_list;
    int        nb_packets;
    int        size;
    int64_t    duration;
    int        abort_request;
    int        serial;
    SDL_mutex* mutex;
    SDL_cond*  cond;
} PacketQueue;

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame
{
    AVFrame*   frame;
    int        serial;
    double     pts;      /* presentation timestamp for the frame */
    double     duration; /* estimated duration of the frame */
    int64_t    pos;      /* byte position of the frame in the input file */
    int        width;
    int        height;
    int        format;
    AVRational sar;
    int        uploaded;
    int        flip_v;
} Frame;

typedef struct FrameQueue
{
    Frame        queue[FRAME_QUEUE_SIZE];
    int          rindex;
    int          windex;
    int          size;
    int          max_size;
    int          keep_last;
    int          rindex_shown;
    SDL_mutex*   mutex;
    SDL_cond*    cond;
    PacketQueue* pktq;
} FrameQueue;

FrameQueue pictq;

void viewer_open(char* path)
{
    zc_log_debug("VIEWER OPEN %s", path);
}

#endif

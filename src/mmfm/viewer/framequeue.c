#ifndef framequeue_h
#define framequeue_h

#include "libavutil/frame.h"
#include <SDL.h>
#include <SDL_thread.h>

#define FRAME_QUEUE_SIZE 16

/* Common struct for handling all types of decoded data and allocated render buffers. */
typedef struct Frame
{
    AVFrame*   frame;    // video/audio frame
    int        serial;   // packetqueue serial / decoder packet serial
    double     pts;      // presentation timestamp for the frame
    double     duration; // estimated duration of the frame
    int64_t    pos;      // byte position of the frame in the input file
    int        width;    // width of frame
    int        height;   // height of frame
    int        format;   // format of the frame, -1 if unknown or unset Values correspond to enum AVPixelFormat for video frames, enum AVSampleFormat for audio)
    AVRational sar;      // sample aspect ratio of frame
    int        uploaded; // frame is uploaded to gpu, do we need this?
    int        flip_v;   // do we have to flip vertically?
} Frame;

typedef struct FrameQueue
{
    Frame      queue[FRAME_QUEUE_SIZE];
    int        rindex;       // read index
    int        windex;       // write index
    int        size;         // actual size of queue
    int        max_size;     // maximus size of queue
    int        keep_last;    // keep last frame
    int        rindex_shown; // read index is already shown
    SDL_mutex* mutex;        // mutex for pushing and reading from different thread
    SDL_cond*  cond;         // condition for pushing and reading from different thread
} FrameQueue;

int    frame_queue_init(FrameQueue* f, int max_size, int keep_last);
void   frame_queue_destroy(FrameQueue* f);
void   frame_queue_push(FrameQueue* f);
void   frame_queue_next(FrameQueue* f);
void   frame_queue_unref_item(Frame* vp);
int    frame_queue_nb_remaining(FrameQueue* f);
Frame* frame_queue_peek(FrameQueue* f);
Frame* frame_queue_peek_next(FrameQueue* f);
Frame* frame_queue_peek_last(FrameQueue* f);
Frame* frame_queue_peek_writable(FrameQueue* f, int abort_request);

#endif

#if __INCLUDE_LEVEL__ == 0

int frame_queue_init(FrameQueue* f, int max_size, int keep_last)
{
    int i;
    memset(f, 0, sizeof(FrameQueue));
    if (!(f->mutex = SDL_CreateMutex()))
    {
	av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
	return AVERROR(ENOMEM);
    }
    if (!(f->cond = SDL_CreateCond()))
    {
	av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
	return AVERROR(ENOMEM);
    }
    f->max_size  = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
	if (!(f->queue[i].frame = av_frame_alloc()))
	    return AVERROR(ENOMEM);
    return 0;
}

void frame_queue_destroy(FrameQueue* f)
{
    int i;
    for (i = 0; i < f->max_size; i++)
    {
	Frame* vp = &f->queue[i];
	frame_queue_unref_item(vp);
	av_frame_free(&vp->frame);
    }
    SDL_DestroyMutex(f->mutex);
    SDL_DestroyCond(f->cond);
}

void frame_queue_push(FrameQueue* f)
{
    if (++f->windex == f->max_size)
	f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

void frame_queue_next(FrameQueue* f)
{
    if (f->keep_last && !f->rindex_shown)
    {
	f->rindex_shown = 1;
	return;
    }
    frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size)
	f->rindex = 0;
    SDL_LockMutex(f->mutex);
    f->size--;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

void frame_queue_unref_item(Frame* vp)
{
    av_frame_unref(vp->frame);
}

/* return the number of undisplayed frames in the queue */
int frame_queue_nb_remaining(FrameQueue* f)
{
    return f->size - f->rindex_shown;
}

Frame* frame_queue_peek(FrameQueue* f)
{
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

Frame* frame_queue_peek_next(FrameQueue* f)
{
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

Frame* frame_queue_peek_last(FrameQueue* f)
{
    return &f->queue[f->rindex];
}

Frame* frame_queue_peek_writable(FrameQueue* f, int abort_request)
{
    /* wait until we have space to put a new frame */
    SDL_LockMutex(f->mutex);
    while (f->size >= f->max_size &&
	   !abort_request)
    {
	SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);

    if (abort_request)
	return NULL;

    return &f->queue[f->windex];
}

#endif

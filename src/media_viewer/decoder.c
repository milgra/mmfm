#ifndef decoder_h
#define decoder_h

#include "framequeue.c"
#include "libavcodec/avcodec.h"
#include "packetqueue.c"
#include <SDL.h>
#include <SDL_thread.h>

typedef struct Decoder
{
    AVPacket* pkt;

    PacketQueue*    pqueue;
    AVCodecContext* codctx;
    int             pkt_serial;
    int             finished;
    int             packet_pending;
    SDL_cond*       empty_queue_cond;
    int64_t         start_pts; // presentation timestamp
    AVRational      start_pts_tb;
    int64_t         next_pts;
    AVRational      next_pts_tb;
    int             av_sync_type;

    // frame drop counter

    int frame_drops_early;
    int frame_drops_late;
} Decoder;

int  decoder_init(Decoder* d, AVCodecContext* codctx, PacketQueue* queue, SDL_cond* empty_queue_cond);
int  decoder_start(Decoder* d);
void decoder_abort(Decoder* d, FrameQueue* fq);
void decoder_destroy(Decoder* d);
int  decoder_decode_frame(Decoder* d, AVFrame* frame);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "zc_log.c"

// do we need this?

static int decoder_reorder_pts = -1;

int decoder_init(Decoder* d, AVCodecContext* codctx, PacketQueue* queue, SDL_cond* empty_queue_cond)
{
    memset(d, 0, sizeof(Decoder));
    d->pkt = av_packet_alloc();
    if (!d->pkt) return AVERROR(ENOMEM);
    d->codctx           = codctx;
    d->pqueue           = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts        = AV_NOPTS_VALUE;
    d->pkt_serial       = -1;
    return 0;
}

int decoder_start(Decoder* d)
{
    packet_queue_start(d->pqueue);
    return 0;
}

void decoder_abort(Decoder* d, FrameQueue* fq)
{
    packet_queue_abort(d->pqueue);
    frame_queue_signal(fq);
}

void decoder_destroy(Decoder* d)
{
    packet_queue_flush(d->pqueue);
    av_packet_free(&d->pkt);
    avcodec_free_context(&d->codctx);
}

int decoder_decode_frame(Decoder* d, AVFrame* frame)
{
    int ret = AVERROR(EAGAIN);

    for (;;)
    {
	if (d->pqueue->serial == d->pkt_serial)
	{
	    do {
		if (d->pqueue->abort_request) return -1;

		switch (d->codctx->codec_type)
		{
		    case AVMEDIA_TYPE_VIDEO:
			ret = avcodec_receive_frame(d->codctx, frame);
			if (ret >= 0)
			{
			    if (decoder_reorder_pts == -1)
			    {
				frame->pts = frame->best_effort_timestamp;
			    }
			    else if (!decoder_reorder_pts)
			    {
				frame->pts = frame->pkt_dts;
			    }
			}
			break;
		    case AVMEDIA_TYPE_AUDIO:
			ret = avcodec_receive_frame(d->codctx, frame);
			if (ret >= 0)
			{
			    AVRational tb = (AVRational){1, frame->sample_rate};
			    if (frame->pts != AV_NOPTS_VALUE)
				frame->pts = av_rescale_q(frame->pts, d->codctx->pkt_timebase, tb);
			    else if (d->next_pts != AV_NOPTS_VALUE)
				frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
			    if (frame->pts != AV_NOPTS_VALUE)
			    {
				d->next_pts    = frame->pts + frame->nb_samples;
				d->next_pts_tb = tb;
			    }
			}
			break;
		    default:
			break;
		}
		if (ret == AVERROR_EOF)
		{
		    d->finished = d->pkt_serial;
		    avcodec_flush_buffers(d->codctx);
		    return 0;
		}

		if (ret >= 0) return 1;

	    } while (ret != AVERROR(EAGAIN));
	}

	do
	{
	    if (d->pqueue->nb_packets == 0) SDL_CondSignal(d->empty_queue_cond);

	    if (d->packet_pending)
	    {
		d->packet_pending = 0;
	    }
	    else
	    {
		int old_serial = d->pkt_serial;

		if (packet_queue_get(d->pqueue, d->pkt, 1, &d->pkt_serial) < 0)
		{
		    return -1;
		}

		if (old_serial != d->pkt_serial)
		{
		    avcodec_flush_buffers(d->codctx);
		    d->finished    = 0;
		    d->next_pts    = d->start_pts;
		    d->next_pts_tb = d->start_pts_tb;
		}
	    }
	    if (d->pqueue->serial == d->pkt_serial) break;
	    av_packet_unref(d->pkt);
	} while (1);

	if (d->codctx->codec_type == AVMEDIA_TYPE_SUBTITLE)
	{
	}
	else
	{
	    if (avcodec_send_packet(d->codctx, d->pkt) == AVERROR(EAGAIN))
	    {
		av_log(d->codctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
		d->packet_pending = 1;
	    }
	    else
	    {
		av_packet_unref(d->pkt);
	    }
	}
    }
}

#endif

#ifndef viewer_h
#define viewer_h

void viewer_open(char* path);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/fifo.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "zc_log.c"
#include <SDL.h>
#include <SDL_thread.h>

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define FRAME_QUEUE_SIZE 16
#define AV_NOSYNC_THRESHOLD 10.0
#define MIN_FRAMES 25

static int     decoder_reorder_pts = -1;
static int     framedrop           = -1; // drop frames on slow cpu
static int     infinite_buffer     = -1;
static int64_t start_time          = AV_NOPTS_VALUE;
static int64_t duration            = AV_NOPTS_VALUE;
static int     autoexit            = -1;
static int     loop                = 1;

enum
{
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct MyAVPacketList
{
    AVPacket* pkt;
    int       serial;
} MyAVPacketList;

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

typedef struct Decoder
{
    AVPacket*       pkt;
    PacketQueue*    queue;
    AVCodecContext* avctx;
    int             pkt_serial;
    int             finished;
    int             packet_pending;
    SDL_cond*       empty_queue_cond;
    int64_t         start_pts;
    AVRational      start_pts_tb;
    int64_t         next_pts;
    AVRational      next_pts_tb;
    SDL_Thread*     decoder_tid;
    int             av_sync_type;
} Decoder;

typedef struct VideoState
{
    AVFormatContext*     ic;
    FrameQueue           pictq;    // frame queue for video
    PacketQueue          videoq;   // pcaket queue for video
    Clock                vidclk;   // clock for video
    Clock                extclk;   // clock for external
    SDL_Thread*          read_tid; // thread for video
    int                  abort_request;
    const AVInputFormat* iformat;
    char*                filename;
    double               max_frame_duration; // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    int                  realtime;           // realtime stream? needed for clock readjustment
    int                  eof;                // end of file reached
    int                  video_stream;
    AVStream*            video_st;
    int                  queue_attachments_req;
    Decoder              viddec;
    SDL_cond*            continue_read_thread;
    int                  av_sync_type;
    double               frame_last_filter_delay;
    int                  frame_drops_early;
    int                  frame_drops_late;
    int                  paused;
    int                  last_paused;
    int                  read_pause_return;
    int                  seek_req;
    int                  seek_flags;
    int64_t              seek_pos;
    int64_t              seek_rel;
    int                  step;
    double               frame_timer;

} VideoState;

AVDictionary *format_optsn, *codec_optsn;

static void set_clock_at(Clock* c, double pts, int serial, double time)
{
    c->pts          = pts;
    c->last_updated = time;
    c->pts_drift    = c->pts - time;
    c->serial       = serial;
}

static void set_clock(Clock* c, double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    set_clock_at(c, pts, serial, time);
}

static void init_clock(Clock* c, int* queue_serial)
{
    c->speed        = 1.0;
    c->paused       = 0;
    c->queue_serial = queue_serial;
    set_clock(c, NAN, -1);
}

static int get_master_sync_type(VideoState* is)
{
    if (is->av_sync_type == AV_SYNC_VIDEO_MASTER)
    {
	if (is->video_st)
	    return AV_SYNC_VIDEO_MASTER;
	else
	    return AV_SYNC_AUDIO_MASTER;
    }
    else
    {
	return AV_SYNC_EXTERNAL_CLOCK;
    }
}

static double get_clock(Clock* c)
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

/* get the current master clock value */
static double get_master_clock(VideoState* is)
{
    double val;

    switch (get_master_sync_type(is))
    {
	case AV_SYNC_VIDEO_MASTER:
	    val = get_clock(&is->vidclk);
	    break;
	default:
	    val = get_clock(&is->extclk);
	    break;
    }
    return val;
}

static int frame_queue_init(FrameQueue* f, PacketQueue* pktq, int max_size, int keep_last)
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
    f->pktq      = pktq;
    f->max_size  = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
	if (!(f->queue[i].frame = av_frame_alloc()))
	    return AVERROR(ENOMEM);
    return 0;
}

static int packet_queue_init(PacketQueue* q)
{
    memset(q, 0, sizeof(PacketQueue));
    q->pkt_list = av_fifo_alloc2(1, sizeof(MyAVPacketList), AV_FIFO_FLAG_AUTO_GROW);
    if (!q->pkt_list)
	return AVERROR(ENOMEM);
    q->mutex = SDL_CreateMutex();
    if (!q->mutex)
    {
	av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
	return AVERROR(ENOMEM);
    }
    q->cond = SDL_CreateCond();
    if (!q->cond)
    {
	av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
	return AVERROR(ENOMEM);
    }
    q->abort_request = 1;
    return 0;
}

static void packet_queue_start(PacketQueue* q)
{
    SDL_LockMutex(q->mutex);
    q->abort_request = 0;
    q->serial++;
    SDL_UnlockMutex(q->mutex);
}

static void packet_queue_flush(PacketQueue* q)
{
    MyAVPacketList pkt1;

    SDL_LockMutex(q->mutex);
    while (av_fifo_read(q->pkt_list, &pkt1, 1) >= 0)
	av_packet_free(&pkt1.pkt);
    q->nb_packets = 0;
    q->size       = 0;
    q->duration   = 0;
    q->serial++;
    SDL_UnlockMutex(q->mutex);
}

static int packet_queue_put_private(PacketQueue* q, AVPacket* pkt)
{
    MyAVPacketList pkt1;
    int            ret;

    if (q->abort_request)
	return -1;

    pkt1.pkt    = pkt;
    pkt1.serial = q->serial;

    ret = av_fifo_write(q->pkt_list, &pkt1, 1);
    if (ret < 0)
	return ret;
    q->nb_packets++;
    q->size += pkt1.pkt->size + sizeof(pkt1);
    q->duration += pkt1.pkt->duration;
    /* XXX: should duplicate packet data in DV case */
    SDL_CondSignal(q->cond);
    return 0;
}

static int packet_queue_put(PacketQueue* q, AVPacket* pkt)
{
    AVPacket* pkt1;
    int       ret;

    pkt1 = av_packet_alloc();
    if (!pkt1)
    {
	av_packet_unref(pkt);
	return -1;
    }
    av_packet_move_ref(pkt1, pkt);

    SDL_LockMutex(q->mutex);
    ret = packet_queue_put_private(q, pkt1);
    SDL_UnlockMutex(q->mutex);

    if (ret < 0)
	av_packet_free(&pkt1);

    return ret;
}

static int packet_queue_put_nullpacket(PacketQueue* q, AVPacket* pkt, int stream_index)
{
    pkt->stream_index = stream_index;
    return packet_queue_put(q, pkt);
}

/* return the number of undisplayed frames in the queue */
static int frame_queue_nb_remaining(FrameQueue* f)
{
    return f->size - f->rindex_shown;
}

/* seek in the stream */
static void stream_seek(VideoState* is, int64_t pos, int64_t rel, int by_bytes)
{
    if (!is->seek_req)
    {
	is->seek_pos = pos;
	is->seek_rel = rel;
	is->seek_flags &= ~AVSEEK_FLAG_BYTE;
	if (by_bytes)
	    is->seek_flags |= AVSEEK_FLAG_BYTE;
	is->seek_req = 1;
	SDL_CondSignal(is->continue_read_thread);
    }
}

/* pause or resume the video */
static void stream_toggle_pause(VideoState* is)
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
    is->paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

static void step_to_next_frame(VideoState* is)
{
    /* if the stream is paused unpause it, then step */
    if (is->paused)
	stream_toggle_pause(is);
    is->step = 1;
}

static int decode_interrupt_cb(void* ctx)
{
    VideoState* is = ctx;
    return is->abort_request;
}

/* check if stream is realtimne */
static int is_realtime(AVFormatContext* s)
{
    if (!strcmp(s->iformat->name, "rtp") || !strcmp(s->iformat->name, "rtsp") || !strcmp(s->iformat->name, "sdp"))
	return 1;

    if (s->pb && (!strncmp(s->url, "rtp:", 4) || !strncmp(s->url, "udp:", 4)))
	return 1;
    return 0;
}

int check_stream_specifier1(AVFormatContext* s, AVStream* st, const char* spec)
{
    int ret = avformat_match_stream_specifier(s, st, spec);
    if (ret < 0)
	av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
    return ret;
}

AVDictionary* filter_codec_opts1(AVDictionary* opts, enum AVCodecID codec_id, AVFormatContext* s, AVStream* st, const AVCodec* codec)
{
    AVDictionary*            ret    = NULL;
    const AVDictionaryEntry* t      = NULL;
    int                      flags  = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM
						 : AV_OPT_FLAG_DECODING_PARAM;
    char                     prefix = 0;
    const AVClass*           cc     = avcodec_get_class();

    if (!codec)
	codec = s->oformat ? avcodec_find_encoder(codec_id)
			   : avcodec_find_decoder(codec_id);

    switch (st->codecpar->codec_type)
    {
	case AVMEDIA_TYPE_VIDEO:
	    prefix = 'v';
	    flags |= AV_OPT_FLAG_VIDEO_PARAM;
	    break;
	case AVMEDIA_TYPE_AUDIO:
	    prefix = 'a';
	    flags |= AV_OPT_FLAG_AUDIO_PARAM;
	    break;
	case AVMEDIA_TYPE_SUBTITLE:
	    prefix = 's';
	    flags |= AV_OPT_FLAG_SUBTITLE_PARAM;
	    break;
    }

    while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)))
    {
	const AVClass* priv_class;
	char*          p = strchr(t->key, ':');

	/* check stream specification in opt name */
	if (p)
	    switch (check_stream_specifier1(s, st, p + 1))
	    {
		case 1: *p = 0; break;
		case 0: continue;
		default: zc_log_error("stream specifier not found");
	    }

	if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
	    !codec ||
	    ((priv_class = codec->priv_class) &&
	     av_opt_find(&priv_class, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ)))
	    av_dict_set(&ret, t->key, t->value, 0);
	else if (t->key[0] == prefix && av_opt_find(&cc, t->key + 1, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ))
	    av_dict_set(&ret, t->key + 1, t->value, 0);

	if (p)
	    *p = ':';
    }
    return ret;
}

static int decoder_init(Decoder* d, AVCodecContext* avctx, PacketQueue* queue, SDL_cond* empty_queue_cond)
{
    memset(d, 0, sizeof(Decoder));
    d->pkt = av_packet_alloc();
    if (!d->pkt)
	return AVERROR(ENOMEM);
    d->avctx            = avctx;
    d->queue            = queue;
    d->empty_queue_cond = empty_queue_cond;
    d->start_pts        = AV_NOPTS_VALUE;
    d->pkt_serial       = -1;
    return 0;
}

static int decoder_start(Decoder* d, int (*fn)(void*), const char* thread_name, void* arg)
{
    packet_queue_start(d->queue);
    d->decoder_tid = SDL_CreateThread(fn, thread_name, arg);
    if (!d->decoder_tid)
    {
	av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
	return AVERROR(ENOMEM);
    }
    return 0;
}

static Frame* frame_queue_peek_writable(FrameQueue* f)
{
    /* wait until we have space to put a new frame */
    SDL_LockMutex(f->mutex);
    while (f->size >= f->max_size &&
	   !f->pktq->abort_request)
    {
	SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);

    if (f->pktq->abort_request)
	return NULL;

    return &f->queue[f->windex];
}

static void frame_queue_push(FrameQueue* f)
{
    if (++f->windex == f->max_size)
	f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

/* return < 0 if aborted, 0 if no packet and > 0 if packet.  */
static int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block, int* serial)
{
    MyAVPacketList pkt1;
    int            ret;

    SDL_LockMutex(q->mutex);

    for (;;)
    {
	if (q->abort_request)
	{
	    ret = -1;
	    break;
	}

	if (av_fifo_read(q->pkt_list, &pkt1, 1) >= 0)
	{
	    q->nb_packets--;
	    q->size -= pkt1.pkt->size + sizeof(pkt1);
	    q->duration -= pkt1.pkt->duration;
	    av_packet_move_ref(pkt, pkt1.pkt);
	    if (serial)
		*serial = pkt1.serial;
	    av_packet_free(&pkt1.pkt);
	    ret = 1;
	    break;
	}
	else if (!block)
	{
	    ret = 0;
	    break;
	}
	else
	{
	    SDL_CondWait(q->cond, q->mutex);
	}
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

static int queue_picture(VideoState* is, AVFrame* src_frame, double pts, double duration, int64_t pos, int serial)
{
    zc_log_debug("queue_picture");

    Frame* vp;

#if defined(DEBUG_SYNC)
    printf("frame_type=%c pts=%0.3f\n", av_get_picture_type_char(src_frame->pict_type), pts);
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

    // set_default_window_size(vp->width, vp->height, vp->sar);

    av_frame_move_ref(vp->frame, src_frame);
    frame_queue_push(&is->pictq);
    return 0;
}

static int decoder_decode_frame(Decoder* d, AVFrame* frame, AVSubtitle* sub)
{
    zc_log_debug("decoder_decode_frame");

    int ret = AVERROR(EAGAIN);

    for (;;)
    {
	zc_log_debug("nb packets %i", d->queue->nb_packets);

	if (d->queue->serial == d->pkt_serial)
	{
	    do {
		if (d->queue->abort_request)
		    return -1;

		switch (d->avctx->codec_type)
		{
		    case AVMEDIA_TYPE_VIDEO:
			ret = avcodec_receive_frame(d->avctx, frame);
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
			ret = avcodec_receive_frame(d->avctx, frame);
			if (ret >= 0)
			{
			    AVRational tb = (AVRational){1, frame->sample_rate};
			    if (frame->pts != AV_NOPTS_VALUE)
				frame->pts = av_rescale_q(frame->pts, d->avctx->pkt_timebase, tb);
			    else if (d->next_pts != AV_NOPTS_VALUE)
				frame->pts = av_rescale_q(d->next_pts, d->next_pts_tb, tb);
			    if (frame->pts != AV_NOPTS_VALUE)
			    {
				d->next_pts    = frame->pts + frame->nb_samples;
				d->next_pts_tb = tb;
			    }
			}
			break;
		}
		if (ret == AVERROR_EOF)
		{
		    d->finished = d->pkt_serial;
		    avcodec_flush_buffers(d->avctx);
		    return 0;
		}
		if (ret >= 0)
		    return 1;
	    } while (ret != AVERROR(EAGAIN));
	}

	do {
	    zc_log_debug("pending %i", d->packet_pending);

	    if (d->queue->nb_packets == 0)
		SDL_CondSignal(d->empty_queue_cond);

	    if (d->packet_pending)
	    {
		d->packet_pending = 0;
	    }
	    else
	    {
		int old_serial = d->pkt_serial;
		zc_log_debug("ee");
		if (packet_queue_get(d->queue, d->pkt, 1, &d->pkt_serial) < 0)
		{

		    zc_log_debug("e");
		    return -1;
		}
		zc_log_debug("eee");

		if (old_serial != d->pkt_serial)
		{
		    avcodec_flush_buffers(d->avctx);
		    d->finished    = 0;
		    d->next_pts    = d->start_pts;
		    d->next_pts_tb = d->start_pts_tb;
		}
	    }
	    if (d->queue->serial == d->pkt_serial)
		break;
	    av_packet_unref(d->pkt);
	} while (1);

	if (d->avctx->codec_type == AVMEDIA_TYPE_SUBTITLE)
	{
	    int got_frame = 0;
	    ret           = avcodec_decode_subtitle2(d->avctx, sub, &got_frame, d->pkt);
	    if (ret < 0)
	    {
		ret = AVERROR(EAGAIN);
	    }
	    else
	    {
		if (got_frame && !d->pkt->data)
		{
		    d->packet_pending = 1;
		}
		ret = got_frame ? 0 : (d->pkt->data ? AVERROR(EAGAIN) : AVERROR_EOF);
	    }
	    av_packet_unref(d->pkt);
	}
	else
	{
	    if (avcodec_send_packet(d->avctx, d->pkt) == AVERROR(EAGAIN))
	    {
		av_log(d->avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
		d->packet_pending = 1;
	    }
	    else
	    {
		av_packet_unref(d->pkt);
	    }
	}
    }
}

static int get_video_frame(VideoState* is, AVFrame* frame)
{
    zc_log_debug("get_video_frame");

    int got_picture = decoder_decode_frame(&is->viddec, frame, NULL);

    if (got_picture < 0)
    {
	zc_log_debug("no picture");
	return -1;
    }

    if (got_picture)
    {
	zc_log_debug("got picture");

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

static int video_thread(void* arg)
{
    zc_log_debug("video thread start");

    VideoState* is    = arg;
    AVFrame*    frame = av_frame_alloc();
    double      pts;
    double      duration;
    int         ret;
    AVRational  tb         = is->video_st->time_base;
    AVRational  frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);

    if (!frame)
	return AVERROR(ENOMEM);

    for (;;)
    {
	ret = get_video_frame(is, frame);

	if (ret < 0) break;

	if (!ret) continue;

	duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
	pts      = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);

	ret = queue_picture(
	    is,
	    frame,
	    pts,
	    duration,
	    frame->pkt_pos,
	    is->viddec.pkt_serial);

	av_frame_unref(frame);

	if (ret < 0) break;
    }

    av_frame_free(&frame);

    return 0;
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(VideoState* is, int stream_index)
{
    zc_log_debug("Stream component open %i", stream_index);

    AVFormatContext* ic = is->ic;
    const AVCodec*   codec;
    AVDictionary*    opts      = NULL;
    AVChannelLayout  ch_layout = {0};

    int ret = -1;

    // check for invalid stream index
    if (stream_index >= 0 || stream_index < ic->nb_streams)
    {
	// allocate codec context
	AVCodecContext* avctx = avcodec_alloc_context3(NULL);

	if (avctx)
	{
	    zc_log_debug("AVCodecContext allocated");

	    ret = avcodec_parameters_to_context(
		avctx,
		ic->streams[stream_index]->codecpar);

	    if (ret >= 0)
	    {
		zc_log_debug("Codec parameters read");

		avctx->pkt_timebase = ic->streams[stream_index]->time_base;

		zc_log_debug("Timebase : %i/%i", avctx->pkt_timebase.num, avctx->pkt_timebase.den);

		codec = avcodec_find_decoder(avctx->codec_id);

		switch (avctx->codec_type)
		{
		    case AVMEDIA_TYPE_AUDIO:
			zc_log_debug("Audio stream");
			break;
		    case AVMEDIA_TYPE_SUBTITLE:
			zc_log_debug("Subtitle stream");
			break;
		    case AVMEDIA_TYPE_VIDEO:
			zc_log_debug("Video stream");
			break;
		}

		if (!codec) zc_log_error("No decoder could be found for codec %s", avcodec_get_name(avctx->codec_id));

		avctx->codec_id = codec->id;
		zc_log_debug("codec id %i", codec->id);

		zc_log_debug("The maximum value for lowres supported by the decoder is %d", codec->max_lowres);

		avctx->flags2 |= AV_CODEC_FLAG2_FAST;
		zc_log_debug("Setting fast decoding");

		opts = filter_codec_opts1(codec_optsn, avctx->codec_id, ic, ic->streams[stream_index], codec);

		if (!av_dict_get(opts, "threads", NULL, 0)) av_dict_set(&opts, "threads", "auto", 0);

		zc_log_debug("Setting auto thread usage");

		ret = avcodec_open2(avctx, codec, &opts);

		if (ret >= 0)
		{
		    is->eof                            = 0;
		    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;

		    switch (avctx->codec_type)
		    {
			case AVMEDIA_TYPE_AUDIO:
			    break;
			case AVMEDIA_TYPE_VIDEO:
			    is->video_stream = stream_index;
			    is->video_st     = ic->streams[stream_index];

			    ret = decoder_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);

			    if (ret >= 0)
			    {
				ret = decoder_start(&is->viddec, video_thread, "video_decoder", is);

				if (ret >= 0)
				{
				    is->queue_attachments_req = 1;
				}
				else
				    zc_log_debug("Cannot start decoder");
			    }
			    else
				zc_log_debug("Cannot init decoder");

			    break;
			case AVMEDIA_TYPE_SUBTITLE:
			    break;
			default:
			    break;
		    }

		    // avcodec_free_context(&avctx);

		    av_channel_layout_uninit(&ch_layout);
		    av_dict_free(&opts);

		    return ret;
		}
		else
		    zc_log_debug("Couldn't open codec");
	    }
	    else
		zc_log_debug("Cannot read codec parameters");
	}
	else
	    ret = AVERROR(ENOMEM);
    }
    else
    {
	zc_log_debug("Invalid stream index");
    }

    return ret;
}

static int stream_has_enough_packets(AVStream* st, int stream_id, PacketQueue* queue)
{
    return stream_id < 0 ||
	   queue->abort_request ||
	   (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
	   queue->nb_packets > (MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0));
}

/* this thread gets the stream from the disk or the network */
static int read_thread(void* arg)
{
    VideoState* is = arg;

    // create mutex
    SDL_mutex* wait_mutex = SDL_CreateMutex();

    if (wait_mutex)
    {
	zc_log_debug("Wait mutex created.");

	int st_index[AVMEDIA_TYPE_NB]; // stream index

	memset(st_index, -1, sizeof(st_index));

	// create packet
	AVPacket* pkt = av_packet_alloc(); // current packet

	if (pkt)
	{
	    zc_log_debug("AVPacket allocated");

	    // create context
	    AVFormatContext* ic = avformat_alloc_context(); // corrent format context

	    if (ic)
	    {
		zc_log_debug("AVFormatContext allocated");

		ic->interrupt_callback.callback = decode_interrupt_cb;
		ic->interrupt_callback.opaque   = is;

		int scan_all_pmts_set = 0; // could get all parameters

		if (!av_dict_get(format_optsn, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE))
		{
		    // in case of scan all not set, set it
		    av_dict_set(&format_optsn, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
		    scan_all_pmts_set = 1;
		}
		else
		    zc_log_debug("");

		int err = avformat_open_input(&ic, is->filename, is->iformat, &format_optsn);

		if (err >= 0)
		{
		    zc_log_debug("Input opened");

		    // why do we have to set this again?
		    if (scan_all_pmts_set)
			av_dict_set(&format_optsn, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

		    // avformat_open_input fills up format_optsn with non-existing parameters

		    const AVDictionaryEntry* t;

		    if ((t = av_dict_get(format_optsn, "", NULL, AV_DICT_IGNORE_SUFFIX)))
		    {
			zc_log_debug("Option %s not found.", t->key);
		    }

		    is->ic = ic;

		    // do we need this?
		    /* if (genpts) */
		    /* 	ic->flags |= AVFMT_FLAG_GENPTS; */

		    // why?
		    av_format_inject_global_side_data(ic);

		    // set eof reached, fix?
		    if (ic->pb) ic->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

		    // get max frame duration for checing timestamp discontinuity
		    is->max_frame_duration = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

		    // check realtimeness, speed adjusment may be needed on realtime streams during refresh
		    is->realtime = is_realtime(ic);

		    // print format info
		    av_dump_format(ic, 0, is->filename, 0);

		    // find video streaam
		    st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(
			ic,
			AVMEDIA_TYPE_VIDEO,
			st_index[AVMEDIA_TYPE_VIDEO],
			-1,
			NULL,
			0);

		    // open streams
		    int ret = -1;
		    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
		    {
			ret = stream_component_open(is, st_index[AVMEDIA_TYPE_VIDEO]);
		    }

		    // set infinite buffer
		    if (infinite_buffer < 0 && is->realtime)
			infinite_buffer = 1;

		    for (;;)
		    {
			// abort if needed
			if (is->abort_request)
			    break;

			// pause
			if (is->paused != is->last_paused)
			{
			    is->last_paused = is->paused;
			    if (is->paused)
				is->read_pause_return = av_read_pause(ic);
			    else
				av_read_play(ic);
			}

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
				av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", is->ic->url);
			    }
			    else
			    {
				if (is->video_stream >= 0)
				    packet_queue_flush(&is->videoq);

				if (is->seek_flags & AVSEEK_FLAG_BYTE)
				{
				    set_clock(&is->extclk, NAN, 0);
				}
				else
				{
				    set_clock(&is->extclk, seek_target / (double) AV_TIME_BASE, 0);
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
				if ((ret = av_packet_ref(pkt, &is->video_st->attached_pic)) < 0)
				    zc_log_error("cannot ref packet");
				packet_queue_put(&is->videoq, pkt);
				packet_queue_put_nullpacket(&is->videoq, pkt, is->video_stream);
			    }
			    is->queue_attachments_req = 0;
			}

			/* if the queue are full, no need to read more */
			if (infinite_buffer < 1 &&
			    (is->videoq.size > MAX_QUEUE_SIZE || stream_has_enough_packets(is->video_st, is->video_stream, &is->videoq)))
			{
			    /* wait 10 ms */
			    SDL_LockMutex(wait_mutex);
			    SDL_CondWaitTimeout(is->continue_read_thread, wait_mutex, 10);
			    SDL_UnlockMutex(wait_mutex);
			    continue;
			}

			if (!is->paused &&
			    (!is->video_st || (is->viddec.finished == is->videoq.serial && frame_queue_nb_remaining(&is->pictq) == 0)))
			{
			    if (loop != 1 && (!loop || --loop))
			    {
				stream_seek(is, start_time != AV_NOPTS_VALUE ? start_time : 0, 0, 0);
			    }
			    else if (autoexit)
			    {
				ret = AVERROR_EOF;
				zc_log_error("autoexit");
			    }
			}

			ret = av_read_frame(ic, pkt);
			if (ret < 0)
			{
			    if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !is->eof)
			    {
				if (is->video_stream >= 0)
				    packet_queue_put_nullpacket(&is->videoq, pkt, is->video_stream);
				is->eof = 1;
			    }
			    if (ic->pb && ic->pb->error)
			    {
				if (autoexit)
				    zc_log_error("autoexit");
				else
				    break;
			    }
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
			int     stream_start_time = ic->streams[pkt->stream_index]->start_time;
			int64_t pkt_ts            = pkt->pts == AV_NOPTS_VALUE ? pkt->dts : pkt->pts;
			int     pkt_in_play_range = duration == AV_NOPTS_VALUE ||
						(pkt_ts - (stream_start_time != AV_NOPTS_VALUE ? stream_start_time : 0)) *
							    av_q2d(ic->streams[pkt->stream_index]->time_base) -
							(double) (start_time != AV_NOPTS_VALUE ? start_time : 0) / 1000000 <=
						    ((double) duration / 1000000);

			if (pkt->stream_index == is->video_stream &&
			    pkt_in_play_range &&
			    !(is->video_st->disposition & AV_DISPOSITION_ATTACHED_PIC))
			{
			    packet_queue_put(&is->videoq, pkt);
			}
			else
			{
			    av_packet_unref(pkt);
			}
		    }
		}
		else
		{
		    zc_log_debug("Cannot open input");
		}
	    }
	    else
		zc_log_debug("Cannot create format context");
	}
	else
	    zc_log_debug("Cannot create packet");
    }
    else
	zc_log_debug("Cannot create mutex %s", SDL_GetError());

    return 0;
}

void viewer_open(char* path)
{
    zc_log_debug("viewer_open %s", path);

    VideoState* is;

    is = av_mallocz(sizeof(VideoState));

    // store filename

    is->filename = av_strdup(path);

    if (is)
    {

	if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
	    zc_log_debug("cannot create frame queue");

	if (packet_queue_init(&is->videoq) < 0)
	    zc_log_debug("cannot create packet queue");

	is->continue_read_thread = SDL_CreateCond();

	if (is->continue_read_thread == NULL)
	    zc_log_error("cannot create sdl cond");

	init_clock(&is->vidclk, &is->videoq.serial);
	init_clock(&is->extclk, &is->extclk.serial);

	is->read_tid = SDL_CreateThread(read_thread, "read_thread", is);
	if (!is->read_tid)
	{
	    zc_log_error("SDL_CreateThread(): %s\n", SDL_GetError());
	}
    }
}

#endif

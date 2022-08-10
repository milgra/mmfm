#ifndef viewer_h
#define viewer_h

#include "zc_bm_rgba.c"

void* viewer_open(char* path);
void  video_refresh(void* opaque, double* remaining_time, bm_rgba_t* bm);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "clock.c"
#include "framequeue.c"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/fifo.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "packetqueue.c"
#include "zc_draw.c"
#include "zc_log.c"
#include <SDL.h>
#include <SDL_thread.h>

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define MIN_FRAMES 25

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN 0.900
#define EXTERNAL_CLOCK_SPEED_MAX 1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10

static int decoder_reorder_pts = -1;
static int framedrop           = -1; // drop frames on slow cpu
static int infinite_buffer     = -1;
static int autoexit            = -1;
static int loop                = 1;
double     rdftspeed1          = 0.02;
uint8_t*   scaledpixels[1];
int        scaledw = 0;
int        scaledh = 0;

enum
{
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct Decoder
{
    AVPacket*       pkt;
    PacketQueue*    queue;
    AVCodecContext* avctx;
    int             pkt_serial;
    int             finished;
    int             packet_pending;
    SDL_cond*       empty_queue_cond;
    int64_t         start_pts; // presentation timestamp
    AVRational      start_pts_tb;
    int64_t         next_pts;
    AVRational      next_pts_tb;
    SDL_Thread*     dec_thread;
    int             av_sync_type;
} Decoder;

typedef struct MediaState
{
    char*            filename;             // filename, to use in functions and for format dumping
    AVFormatContext* format;               // format context, used for aspect ratio, framerate, seek calcualtions
    SDL_Thread*      read_thread;          // thread for video
    SDL_cond*        continue_read_thread; // conditional for read thread start/stop

    Clock vidclk; // clock for video
    Clock audclk; // clock for audio
    Clock extclk; // clock for external

    FrameQueue  vidfq; // frame queue for video, contains complete frames
    PacketQueue vidpq; // pcaket queue for video, packets contain compressed data, one frame per packet in case of video

    Decoder viddec; // video decoder

    AVStream* vidst;       // video stream
    int       vidst_index; // video stream index

    int check_attachment; // check for video attachment after open and seek

    int    abort_request;
    double max_frame_duration; // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity
    int    realtime;           // realtime stream? needed for clock readjustment
    int    eof;                // end of file reached

    int     av_sync_type;
    double  frame_last_filter_delay;
    int     frame_drops_early;
    int     frame_drops_late;
    int     paused;
    int     last_paused;
    int     read_pause_return; // read pause return value
    int     seek_req;
    int     seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;
    int     step;
    double  frame_timer;
    int     force_refresh;
    double  last_vis_time;

    struct SwsContext* img_convert_ctx; // image conversion context for scaling on upload
} MediaState;

AVDictionary *format_optsn, *codec_optsn;

static void check_external_clock_speed(MediaState* ms)
{
    if (ms->vidst_index >= 0 && ms->vidpq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES)
    {
	clock_set_speed(&ms->extclk, FFMAX(EXTERNAL_CLOCK_SPEED_MIN, ms->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
    }
    else if (ms->vidst_index < 0 || ms->vidpq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)
    {
	clock_set_speed(&ms->extclk, FFMIN(EXTERNAL_CLOCK_SPEED_MAX, ms->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
    }
    else
    {
	double speed = ms->extclk.speed;
	if (speed != 1.0)
	    clock_set_speed(&ms->extclk, speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
    }
}

static int get_master_sync_type(MediaState* ms)
{
    if (ms->av_sync_type == AV_SYNC_VIDEO_MASTER)
    {
	if (ms->vidst)
	    return AV_SYNC_VIDEO_MASTER;
	else
	    return AV_SYNC_AUDIO_MASTER;
    }
    else
    {
	return AV_SYNC_EXTERNAL_CLOCK;
    }
}

/* get the current master clock value */
static double get_master_clock(MediaState* ms)
{
    double val;

    switch (get_master_sync_type(ms))
    {
	case AV_SYNC_VIDEO_MASTER:
	    val = clock_get(&ms->vidclk);
	    break;
	default:
	    val = clock_get(&ms->extclk);
	    break;
    }
    return val;
}

/* seek in the stream */
static void stream_seek(MediaState* ms, int64_t pos, int64_t rel, int by_bytes)
{
    if (!ms->seek_req)
    {
	ms->seek_pos = pos;
	ms->seek_rel = rel;
	ms->seek_flags &= ~AVSEEK_FLAG_BYTE;
	if (by_bytes) ms->seek_flags |= AVSEEK_FLAG_BYTE;
	ms->seek_req = 1;
	SDL_CondSignal(ms->continue_read_thread);
    }
}

/* pause or resume the video */
static void stream_toggle_pause(MediaState* ms)
{
    if (ms->paused)
    {
	ms->frame_timer += av_gettime_relative() / 1000000.0 - ms->vidclk.last_updated;
	if (ms->read_pause_return != AVERROR(ENOSYS))
	{
	    ms->vidclk.paused = 0;
	}
	clock_set(&ms->vidclk, clock_get(&ms->vidclk), ms->vidclk.serial);
    }
    clock_set(&ms->extclk, clock_get(&ms->extclk), ms->extclk.serial);
    ms->paused = ms->vidclk.paused = ms->extclk.paused = !ms->paused;
}

static void step_to_next_frame(MediaState* ms)
{
    /* if the stream is paused unpause it, then step */
    if (ms->paused)
	stream_toggle_pause(ms);
    ms->step = 1;
}

static int decode_interrupt_cb(void* ctx)
{
    MediaState* ms = ctx;
    return ms->abort_request;
}

/* check if stream is realtimne */
static int is_realtime(AVFormatContext* format)
{
    if (!strcmp(format->iformat->name, "rtp") || !strcmp(format->iformat->name, "rtsp") || !strcmp(format->iformat->name, "sdp"))
	return 1;

    if (format->pb && (!strncmp(format->url, "rtp:", 4) || !strncmp(format->url, "udp:", 4)))
	return 1;
    return 0;
}

int check_stream_specifier1(AVFormatContext* format, AVStream* st, const char* spec)
{
    int ret = avformat_match_stream_specifier(format, st, spec);
    if (ret < 0)
	av_log(format, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
    return ret;
}

AVDictionary* filter_codec_opts1(AVDictionary* opts, enum AVCodecID codec_id, AVFormatContext* format, AVStream* st, const AVCodec* codec)
{
    AVDictionary*            ret    = NULL;
    const AVDictionaryEntry* t      = NULL;
    int                      flags  = format->oformat ? AV_OPT_FLAG_ENCODING_PARAM
						      : AV_OPT_FLAG_DECODING_PARAM;
    char                     prefix = 0;
    const AVClass*           cc     = avcodec_get_class();

    if (!codec)
	codec = format->oformat ? avcodec_find_encoder(codec_id)
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
	default:
	    break;
    }

    while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)))
    {
	const AVClass* priv_class;
	char*          p = strchr(t->key, ':');

	/* check stream specification in opt name */
	if (p)
	    switch (check_stream_specifier1(format, st, p + 1))
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
    d->dec_thread = SDL_CreateThread(fn, thread_name, arg);
    if (!d->dec_thread)
    {
	av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
	return AVERROR(ENOMEM);
    }
    return 0;
}

static void decoder_abort(Decoder* d, FrameQueue* fq)
{
    packet_queue_abort(d->queue);
    frame_queue_signal(fq);
    SDL_WaitThread(d->dec_thread, NULL);
    d->dec_thread = NULL;
    packet_queue_flush(d->queue);
}

static void decoder_destroy(Decoder* d)
{
    av_packet_free(&d->pkt);
    avcodec_free_context(&d->avctx);
}

static int queue_picture(MediaState* ms, AVFrame* src_frame, double pts, double duration, int64_t pos, int serial)
{
    zc_log_debug("queue_picture");

    Frame* vp;

#if defined(DEBUG_SYNC)
    printf("frame_type=%c pts=%0.3f\n", av_get_picture_type_char(src_frame->pict_type), pts);
#endif

    if (!(vp = frame_queue_peek_writable(&ms->vidfq, ms->vidpq.abort_request)))
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
    frame_queue_push(&ms->vidfq);
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
		    default:
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

static int get_video_frame(MediaState* ms, AVFrame* frame)
{
    zc_log_debug("get_video_frame");

    int got_picture = decoder_decode_frame(&ms->viddec, frame, NULL);

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
	    dpts = av_q2d(ms->vidst->time_base) * frame->pts;

	frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(ms->format, ms->vidst, frame);

	if (framedrop > 0 || (framedrop && get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER))
	{
	    if (frame->pts != AV_NOPTS_VALUE)
	    {
		double diff = dpts - get_master_clock(ms);
		if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
		    diff - ms->frame_last_filter_delay < 0 &&
		    ms->viddec.pkt_serial == ms->vidclk.serial &&
		    ms->vidpq.nb_packets)
		{
		    ms->frame_drops_early++;
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

    MediaState* ms    = arg;
    AVFrame*    frame = av_frame_alloc();
    double      pts;
    double      duration;
    int         ret;
    AVRational  tb         = ms->vidst->time_base;
    AVRational  frame_rate = av_guess_frame_rate(ms->format, ms->vidst, NULL);

    if (!frame) return AVERROR(ENOMEM);

    for (;;)
    {
	ret = get_video_frame(ms, frame);

	if (ret < 0) break;

	if (!ret) continue;

	duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
	pts      = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);

	ret = queue_picture(
	    ms,
	    frame,
	    pts,
	    duration,
	    frame->pkt_pos,
	    ms->viddec.pkt_serial);

	av_frame_unref(frame);

	if (ret < 0) break;
    }

    av_frame_free(&frame);

    return 0;
}

/* open a given stream. Return 0 if OK */
static int stream_component_open(MediaState* ms, int stream_index)
{
    zc_log_debug("Stream component open %i", stream_index);

    AVFormatContext* format = ms->format;
    const AVCodec*   codec;
    AVDictionary*    opts      = NULL;
    AVChannelLayout  ch_layout = {0};

    int ret = -1;

    // check for invalid stream index
    if (stream_index >= 0 || stream_index < format->nb_streams)
    {
	// allocate codec context
	AVCodecContext* avctx = avcodec_alloc_context3(NULL);

	if (avctx)
	{
	    zc_log_debug("AVCodecContext allocated");

	    ret = avcodec_parameters_to_context(
		avctx,
		format->streams[stream_index]->codecpar);

	    if (ret >= 0)
	    {
		zc_log_debug("Codec parameters read");

		avctx->pkt_timebase = format->streams[stream_index]->time_base;

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
		    default:
			break;
		}

		if (!codec) zc_log_error("No decoder could be found for codec %s", avcodec_get_name(avctx->codec_id));

		avctx->codec_id = codec->id;
		zc_log_debug("codec id %i", codec->id);

		zc_log_debug("The maximum value for lowres supported by the decoder is %d", codec->max_lowres);

		avctx->flags2 |= AV_CODEC_FLAG2_FAST;
		zc_log_debug("Setting fast decoding");

		opts = filter_codec_opts1(codec_optsn, avctx->codec_id, format, format->streams[stream_index], codec);

		if (!av_dict_get(opts, "threads", NULL, 0)) av_dict_set(&opts, "threads", "auto", 0);

		zc_log_debug("Setting auto thread usage");

		ret = avcodec_open2(avctx, codec, &opts);

		if (ret >= 0)
		{
		    ms->eof                                = 0;
		    format->streams[stream_index]->discard = AVDISCARD_DEFAULT;

		    switch (avctx->codec_type)
		    {
			case AVMEDIA_TYPE_AUDIO:
			    break;
			case AVMEDIA_TYPE_VIDEO:
			    ms->vidst_index = stream_index;
			    ms->vidst       = format->streams[stream_index];

			    ret = decoder_init(&ms->viddec, avctx, &ms->vidpq, ms->continue_read_thread);

			    if (ret >= 0)
			    {
				ret = decoder_start(&ms->viddec, video_thread, "video_decoder", ms);

				if (ret >= 0)
				{
				    ms->check_attachment = 1;
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

static void stream_component_close(MediaState* ms, int stream_index)
{
    AVFormatContext*   format = ms->format;
    AVCodecParameters* codecpar;

    if (stream_index < 0 || stream_index >= format->nb_streams) return;
    codecpar = format->streams[stream_index]->codecpar;

    switch (codecpar->codec_type)
    {
	case AVMEDIA_TYPE_VIDEO:
	    decoder_abort(&ms->viddec, &ms->vidfq);
	    decoder_destroy(&ms->viddec);
	    break;
	default:
	    break;
    }

    format->streams[stream_index]->discard = AVDISCARD_ALL;
    switch (codecpar->codec_type)
    {
	case AVMEDIA_TYPE_VIDEO:
	    ms->vidst       = NULL;
	    ms->vidst_index = -1;
	    break;
	default:
	    break;
    }
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
    MediaState* ms = arg;

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
	    AVFormatContext* format = avformat_alloc_context(); // corrent format context

	    if (format)
	    {
		zc_log_debug("AVFormatContext allocated");

		format->interrupt_callback.callback = decode_interrupt_cb;
		format->interrupt_callback.opaque   = ms;

		int scan_all_pmts_set = 0; // could get all parameters

		if (!av_dict_get(format_optsn, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE))
		{
		    // in case of scan all not set, set it
		    av_dict_set(&format_optsn, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
		    scan_all_pmts_set = 1;
		}
		else
		    zc_log_debug("");

		int err = avformat_open_input(&format, ms->filename, NULL, &format_optsn);

		if (err >= 0)
		{
		    zc_log_debug("Input opened, format name");

		    // why do we have to set this again?
		    if (scan_all_pmts_set)
			av_dict_set(&format_optsn, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

		    // avformat_open_input fills up format_optsn with non-existing parameters

		    const AVDictionaryEntry* t;

		    if ((t = av_dict_get(format_optsn, "", NULL, AV_DICT_IGNORE_SUFFIX)))
		    {
			zc_log_debug("Option %s not found.", t->key);
		    }

		    ms->format = format;

		    // do we need this?
		    /* if (genpts) */
		    /* 	format->flags |= AVFMT_FLAG_GENPTS; */

		    // why?
		    av_format_inject_global_side_data(format);

		    // set eof reached, fix?
		    if (format->pb) format->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

		    // get max frame duration for checing timestamp discontinuity
		    ms->max_frame_duration = (format->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

		    // check realtimeness, speed adjusment may be needed on realtime streams during refresh
		    ms->realtime = is_realtime(format);

		    // print format info
		    av_dump_format(format, 0, ms->filename, 0);

		    // find video streaam
		    st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(
			format,
			AVMEDIA_TYPE_VIDEO,
			st_index[AVMEDIA_TYPE_VIDEO],
			-1,
			NULL,
			0);

		    // open streams
		    int ret = -1;
		    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0)
		    {
			ret = stream_component_open(ms, st_index[AVMEDIA_TYPE_VIDEO]);
		    }

		    // set infinite buffer
		    if (infinite_buffer < 0 && ms->realtime)
			infinite_buffer = 1;

		    for (;;)
		    {
			// abort if needed
			if (ms->abort_request)
			    break;

			// pause
			if (ms->paused != ms->last_paused)
			{
			    ms->last_paused = ms->paused;
			    if (ms->paused)
				ms->read_pause_return = av_read_pause(format);
			    else
				av_read_play(format);
			}

			if (ms->seek_req)
			{
			    int64_t seek_target = ms->seek_pos;
			    int64_t seek_min    = ms->seek_rel > 0 ? seek_target - ms->seek_rel + 2 : INT64_MIN;
			    int64_t seek_max    = ms->seek_rel < 0 ? seek_target - ms->seek_rel - 2 : INT64_MAX;
			    // FIXME the +-2 is due to rounding being not done in the correct direction in generation
			    //      of the seek_pos/seek_rel variables

			    ret = avformat_seek_file(ms->format, -1, seek_min, seek_target, seek_max, ms->seek_flags);
			    if (ret < 0)
			    {
				av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", ms->format->url);
			    }
			    else
			    {
				if (ms->vidst_index >= 0)
				    packet_queue_flush(&ms->vidpq);

				if (ms->seek_flags & AVSEEK_FLAG_BYTE)
				{
				    clock_set(&ms->extclk, NAN, 0);
				}
				else
				{
				    clock_set(&ms->extclk, seek_target / (double) AV_TIME_BASE, 0);
				}
			    }
			    ms->seek_req         = 0;
			    ms->check_attachment = 1;
			    ms->eof              = 0;
			    if (ms->paused)
				step_to_next_frame(ms);
			}

			if (ms->check_attachment)
			{
			    if (ms->vidst && ms->vidst->disposition & AV_DISPOSITION_ATTACHED_PIC)
			    {
				if ((ret = av_packet_ref(pkt, &ms->vidst->attached_pic)) < 0)
				    zc_log_error("cannot ref packet");
				packet_queue_put(&ms->vidpq, pkt);
				packet_queue_put_nullpacket(&ms->vidpq, pkt, ms->vidst_index);
			    }
			    ms->check_attachment = 0;
			}

			/* if the queue are full, no need to read more */
			if (infinite_buffer < 1 &&
			    (ms->vidpq.size > MAX_QUEUE_SIZE || stream_has_enough_packets(ms->vidst, ms->vidst_index, &ms->vidpq)))
			{
			    /* wait 10 ms */
			    SDL_LockMutex(wait_mutex);
			    SDL_CondWaitTimeout(ms->continue_read_thread, wait_mutex, 10);
			    SDL_UnlockMutex(wait_mutex);
			    continue;
			}

			if (!ms->paused &&
			    (!ms->vidst || (ms->viddec.finished == ms->vidpq.serial && frame_queue_nb_remaining(&ms->vidfq) == 0)))
			{
			    if (loop != 1 && (!loop || --loop))
			    {
				stream_seek(ms, 0, 0, 0);
			    }
			    else if (autoexit)
			    {
				ret = AVERROR_EOF;
				zc_log_error("autoexit");
			    }
			}

			ret = av_read_frame(format, pkt);
			if (ret < 0)
			{
			    if ((ret == AVERROR_EOF || avio_feof(format->pb)) && !ms->eof)
			    {
				if (ms->vidst_index >= 0)
				    packet_queue_put_nullpacket(&ms->vidpq, pkt, ms->vidst_index);
				ms->eof = 1;
			    }
			    if (format->pb && format->pb->error)
			    {
				if (autoexit)
				    zc_log_error("autoexit");
				else
				    break;
			    }
			    SDL_LockMutex(wait_mutex);
			    SDL_CondWaitTimeout(ms->continue_read_thread, wait_mutex, 10);
			    SDL_UnlockMutex(wait_mutex);
			    continue;
			}
			else
			{
			    ms->eof = 0;
			}
			/* check if packet is in play range specified by user, then queue, otherwise discard */

			if (pkt->stream_index == ms->vidst_index &&
			    !(ms->vidst->disposition & AV_DISPOSITION_ATTACHED_PIC))
			{
			    packet_queue_put(&ms->vidpq, pkt);
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

void* viewer_open(char* path)
{
    zc_log_debug("viewer_open %s", path);

    MediaState* ms;

    ms = av_mallocz(sizeof(MediaState));

    // store filename

    ms->filename = av_strdup(path);

    if (ms)
    {

	if (frame_queue_init(&ms->vidfq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
	    zc_log_debug("cannot create frame queue");

	if (packet_queue_init(&ms->vidpq) < 0)
	    zc_log_debug("cannot create packet queue");

	ms->continue_read_thread = SDL_CreateCond();

	if (ms->continue_read_thread == NULL) zc_log_error("cannot create sdl cond");

	clock_init(&ms->vidclk, &ms->vidpq.serial);
	clock_init(&ms->extclk, &ms->extclk.serial);

	ms->read_thread = SDL_CreateThread(read_thread, "read_thread", ms);
	if (!ms->read_thread)
	{
	    zc_log_error("SDL_CreateThread(): %s\n", SDL_GetError());
	}
    }

    return ms;
}

static void viewer_close(MediaState* ms)
{
    /* XXX: use a special url_shutdown call to abort parse cleanly */
    ms->abort_request = 1;
    SDL_WaitThread(ms->read_thread, NULL);

    /* close each stream */
    if (ms->vidst >= 0)
	stream_component_close(ms, ms->vidst_index);

    avformat_close_input(&ms->format);

    packet_queue_destroy(&ms->vidpq);

    /* free all pictures */
    frame_queue_destroy(&ms->vidfq);
    SDL_DestroyCond(ms->continue_read_thread);

    sws_freeContext(ms->img_convert_ctx);

    av_free(ms->filename);
    av_free(ms);
}

static void sync_clock_to_slave(Clock* c, Clock* slave)
{
    double clock       = clock_get(c);
    double slave_clock = clock_get(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
	clock_set(c, slave_clock, slave->serial);
}

static void update_video_pts(MediaState* ms, double pts, int64_t pos, int serial)
{
    /* update current video pts */
    clock_set(&ms->vidclk, pts, serial);
    sync_clock_to_slave(&ms->extclk, &ms->vidclk);
}
static double compute_target_delay(double delay, MediaState* ms)
{
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER)
    {
	/* if video is slave, we try to correct big delays by
	   duplicating or deleting a frame */
	diff = clock_get(&ms->vidclk) - get_master_clock(ms);

	/* skip or repeat frame. We take into account the
	   delay to compute the threshold. I still don't know
	   if it is the best guess */
	sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
	if (!isnan(diff) && fabs(diff) < ms->max_frame_duration)
	{
	    if (diff <= -sync_threshold)
		delay = FFMAX(0, delay + diff);
	    else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
		delay = delay + diff;
	    else if (diff >= sync_threshold)
		delay = 2 * delay;
	}
    }

    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n", delay, -diff);

    return delay;
}

static double vp_duration(MediaState* ms, Frame* vp, Frame* nextvp)
{
    if (vp->serial == nextvp->serial)
    {
	double duration = nextvp->pts - vp->pts;
	if (isnan(duration) || duration <= 0 || duration > ms->max_frame_duration)
	    return vp->duration;
	else
	    return duration;
    }
    else
    {
	return 0.0;
    }
}

static unsigned sws_flags = SWS_BICUBIC;

static int upload_texture(SDL_Texture** tex, AVFrame* frame, struct SwsContext** img_convert_ctx, bm_rgba_t* bm)
{
    int ret = 0;

    zc_log_debug("upload texture frame %i %i bitmap %i %i", frame->width, frame->height, bm->w, bm->h);

    if (frame->width > 0 && frame->height > 0)
    {

	*img_convert_ctx = sws_getCachedContext(*img_convert_ctx, frame->width, frame->height, frame->format, bm->w, bm->h, AV_PIX_FMT_BGR32, sws_flags, NULL, NULL, NULL);

	if (*img_convert_ctx != NULL)
	{
	    // uint8_t* pixels[4];
	    int pitch[4];

	    pitch[0] = bm->w * 4;
	    sws_scale(*img_convert_ctx, (const uint8_t* const*) frame->data, frame->linesize, 0, frame->height, scaledpixels, pitch);

	    if (bm)
	    {
		gfx_insert_rgb(bm, scaledpixels[0], bm->w, bm->h, 0, 0);
	    }
	    else
	    {
		// gl_upload_to_texture(index, 0, 0, w, h, scaledpixelsn[0]);
	    }
	}
    }
    else
	zc_log_debug("invalid frame");
    return ret;
}

static void video_image_display(MediaState* ms, bm_rgba_t* bm)
{
    Frame* vp;
    // Frame*   sp = NULL;
    // SDL_Rect rect;

    vp = frame_queue_peek_last(&ms->vidfq);

    if (!vp->uploaded)
    {
	if (upload_texture(NULL, vp->frame, &ms->img_convert_ctx, bm) < 0)
	{
	    return;
	}
	vp->uploaded = 1;
	vp->flip_v   = vp->frame->linesize[0] < 0;
    }
}

/* display the current picture, if any */
static void video_display(MediaState* ms, bm_rgba_t* bm)
{
    if (ms->vidst) video_image_display(ms, bm);
}

/* called to display each frame */
void video_refresh(void* opaque, double* remaining_time, bm_rgba_t* bm)
{
    MediaState* ms = opaque;
    double      time;

    // Frame *sp, *sp2;

    if (!ms->paused && get_master_sync_type(ms) == AV_SYNC_EXTERNAL_CLOCK && ms->realtime)
	check_external_clock_speed(ms);

    time = av_gettime_relative() / 1000000.0;
    if (ms->force_refresh || ms->last_vis_time + rdftspeed1 < time)
    {
	video_display(ms, bm);
	ms->last_vis_time = time;
    }
    *remaining_time = FFMIN(*remaining_time, ms->last_vis_time + rdftspeed1 - time);

    if (ms->vidst)
    {
	if (scaledw != bm->w || scaledh != bm->h)
	{
	    if (scaledw > 0 || scaledh > 0) free(scaledpixels[0]);

	    scaledw         = bm->w;
	    scaledh         = bm->h;
	    scaledpixels[0] = malloc(bm->w * bm->h * 4);
	}

    retry:
	if (frame_queue_nb_remaining(&ms->vidfq) == 0)
	{
	    // nothing to do, no picture to display in the queue
	}
	else
	{
	    double last_duration, duration, delay;
	    Frame *vp, *lastvp;

	    /* dequeue the picture */
	    lastvp = frame_queue_peek_last(&ms->vidfq);
	    vp     = frame_queue_peek(&ms->vidfq);

	    if (vp->serial != ms->vidpq.serial)
	    {
		frame_queue_next(&ms->vidfq);
		goto retry;
	    }

	    if (lastvp->serial != vp->serial)
		ms->frame_timer = av_gettime_relative() / 1000000.0;

	    if (ms->paused)
		goto display;

	    /* compute nominal last_duration */
	    last_duration = vp_duration(ms, lastvp, vp);
	    delay         = compute_target_delay(last_duration, ms);

	    time = av_gettime_relative() / 1000000.0;
	    if (time < ms->frame_timer + delay)
	    {
		*remaining_time = FFMIN(ms->frame_timer + delay - time, *remaining_time);
		goto display;
	    }

	    ms->frame_timer += delay;
	    if (delay > 0 && time - ms->frame_timer > AV_SYNC_THRESHOLD_MAX)
		ms->frame_timer = time;

	    SDL_LockMutex(ms->vidfq.mutex);
	    if (!isnan(vp->pts))
		update_video_pts(ms, vp->pts, vp->pos, vp->serial);
	    SDL_UnlockMutex(ms->vidfq.mutex);

	    if (frame_queue_nb_remaining(&ms->vidfq) > 1)
	    {
		Frame* nextvp = frame_queue_peek_next(&ms->vidfq);
		duration      = vp_duration(ms, vp, nextvp);
		if (!ms->step && (framedrop > 0 || (framedrop && get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER)) && time > ms->frame_timer + duration)
		{
		    ms->frame_drops_late++;
		    frame_queue_next(&ms->vidfq);
		    goto retry;
		}
	    }

	    frame_queue_next(&ms->vidfq);
	    ms->force_refresh = 1;

	    if (ms->step && !ms->paused)
		stream_toggle_pause(ms);
	}
    display:
	/* display picture */
	if (ms->force_refresh && ms->vidfq.rindex_shown) video_display(ms, bm);
    }
    ms->force_refresh = 0;
}

#endif

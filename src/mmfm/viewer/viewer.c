/*
  Media Viewer namespace

  viewer_open creates a read thread that reads up packets from the given media.
  The read thread creates one or multiple decoder threads depending on the media streams. Decoders decode packets to frames.
  After the threads are started and reading/decoding is running a final frame can be requested with video_refresh, if there is
  a frame available, it gets copied to the given bitmap.
  Sound streams are played by SDL audio.

 */

#ifndef viewer_h
#define viewer_h

#include "zc_bm_rgba.c"

void* viewer_open(char* path);
void  video_refresh(void* opaque, double* remaining_time, bm_rgba_t* bm);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "clock.c"
#include "decoder.c"
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

static int framedrop = -1; // drop frames on slow cpu

double   rdftspeed1 = 0.02;
uint8_t* scaledpixels[1];
int      scaledw = 0;
int      scaledh = 0;

enum
{
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct MediaState
{
    char*            filename;             // filename, to use in functions and for format dumping
    AVFormatContext* format;               // format context, used for aspect ratio, framerate, seek calcualtions
    SDL_Thread*      read_thread;          // thread for reading packets
    SDL_cond*        continue_read_thread; // conditional for read thread start/stop

    SDL_Thread* video_thread; // thread for video decoding

    Clock vidclk; // clock for video
    Clock audclk; // clock for audio
    Clock extclk; // clock for external

    FrameQueue  vidfq; // frame queue for video, contains complete frames
    PacketQueue vidpq; // pcaket queue for video, packets contain compressed data, one frame per packet in case of video

    Decoder viddec; // video decoder

    AVStream* vidst;       // video stream
    int       vidst_index; // video stream index

    double max_frame_duration; // maximum duration of a frame - above this, we consider the jump a timestamp discontinuity. used for target duration, frame duration

    // flags

    int check_attachment; // check for video attachment after open and seek
    int step_frame;       // indicates that we should step to next frame instead of playing
    int abort_request;    // flag for aborting the read thread
    int eof;              // end of file reached

    // pause state

    int paused;            // pause state
    int last_paused;       // store prevuius pause state
    int read_pause_return; // read pause return value

    // seek

    int     seek_req;
    int     seek_flags;
    int64_t seek_pos;
    int64_t seek_rel;

    // display related

    double frame_timer;   // timer for frame display
    int    force_refresh; // force refresh for video display
    double last_vis_time;

    struct SwsContext* img_convert_ctx; // image conversion context for scaling on upload
} MediaState;

// timing related

static int viewer_get_master_sync_type(MediaState* ms)
{
    if (ms->vidst)
	return AV_SYNC_VIDEO_MASTER;
    else
	return AV_SYNC_AUDIO_MASTER;
}

static double viewer_get_master_clock(MediaState* ms)
{
    double val;

    switch (viewer_get_master_sync_type(ms))
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

// stream related

static void viewer_stream_seek(MediaState* ms, int64_t pos, int64_t rel, int by_bytes)
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

static void viewer_stream_toggle_pause(MediaState* ms)
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

static void viewer_step_to_next_frame(MediaState* ms)
{
    /* if the stream is paused unpause it, then step */
    if (ms->paused) viewer_stream_toggle_pause(ms);
    ms->step_frame = 1;
}

static int viewer_video_decode_thread(void* arg)
{
    zc_log_debug("Video decoder thread started.");

    MediaState* ms    = arg;
    AVFrame*    frame = av_frame_alloc();
    int         ret   = -1;

    double     pts;
    double     duration;
    AVRational tb         = ms->vidst->time_base;
    AVRational frame_rate = av_guess_frame_rate(ms->format, ms->vidst, NULL);

    zc_log_debug("time base %f framerate %f", (float) tb.num / (float) tb.den, (float) frame_rate.num / (float) frame_rate.den);

    if (frame)
    {
	/* decode frames forever */
	for (;;)
	{
	    // decode next frame
	    ret = decoder_decode_frame(&ms->viddec, frame);

	    // check frame drop
	    if (ret >= 0)
	    {
		double dpts = NAN;
		if (frame->pts != AV_NOPTS_VALUE) dpts = av_q2d(ms->vidst->time_base) * frame->pts;
		if (framedrop > 0 || (framedrop && viewer_get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER))
		{
		    if (frame->pts != AV_NOPTS_VALUE)
		    {
			double diff = dpts - viewer_get_master_clock(ms);
			if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD &&
			    ms->viddec.pkt_serial == ms->vidclk.serial &&
			    ms->vidpq.nb_packets)
			{
			    ms->viddec.frame_drops_early++;
			    av_frame_unref(frame);
			    continue; // we have to drop this frame
			}
		    }
		}
	    }

	    if (ret > 0) // we got frame!
	    {
		// calculate aspect ratio
		frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(ms->format, ms->vidst, frame);

		duration = (frame_rate.num && frame_rate.den ? av_q2d((AVRational){frame_rate.den, frame_rate.num}) : 0);
		pts      = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);

		printf("frame_type=%c pts=%0.3f\n", av_get_picture_type_char(frame->pict_type), pts);

		// push to queue
		Frame* qframe = frame_queue_peek_writable(&ms->vidfq, ms->vidpq.abort_request);

		if (qframe)
		{
		    qframe->sar      = frame->sample_aspect_ratio;
		    qframe->uploaded = 0;

		    qframe->width  = frame->width;
		    qframe->height = frame->height;
		    qframe->format = frame->format;

		    qframe->pts      = pts;
		    qframe->duration = duration;
		    qframe->pos      = frame->pkt_pos;
		    qframe->serial   = ms->viddec.pkt_serial;

		    // set_default_window_size(qframe->width, qframe->height, qframe->sar);

		    av_frame_move_ref(qframe->frame, frame);
		    frame_queue_push(&ms->vidfq);

		    av_frame_unref(frame);
		}
		else // we have no space in queue
		{
		    break;
		}
	    }
	    else if (ret == 0) // no frame yet
	    {
		continue;
	    }
	    else if (ret < 0) // shit happened
	    {
		break;
	    }
	}

	av_frame_free(&frame);
    }
    else return AVERROR(ENOMEM);

    return 0;
}

static int viewer_stream_open(MediaState* ms, int stream_index)
{
    zc_log_debug("Opening stream %i", stream_index);

    AVFormatContext* format = ms->format;
    int              ret    = -1;

    if (stream_index >= 0 || stream_index < format->nb_streams)
    {
	AVCodecContext* codecctx = avcodec_alloc_context3(NULL);

	if (codecctx)
	{
	    ret = avcodec_parameters_to_context(
		codecctx,
		format->streams[stream_index]->codecpar);

	    if (ret >= 0)
	    {
		codecctx->pkt_timebase = format->streams[stream_index]->time_base;

		const AVCodec* codec = avcodec_find_decoder(codecctx->codec_id);

		zc_log_debug("Timebase : %i/%i Codec type %i Codec id", codecctx->pkt_timebase.num, codecctx->pkt_timebase.den, codecctx->codec_type, codecctx->codec_id);

		if (codec)
		{
		    codecctx->codec_id = codec->id;
		    codecctx->flags2 |= AV_CODEC_FLAG2_FAST; /* enable fast decoding */

		    AVDictionary* opts = NULL;
		    av_dict_set(&opts, "threads", "auto", 0);

		    ret = avcodec_open2(codecctx, codec, &opts);

		    if (ret >= 0)
		    {
			ms->eof = 0;

			format->streams[stream_index]->discard = AVDISCARD_DEFAULT; // discard useless packets

			switch (codecctx->codec_type)
			{
			    case AVMEDIA_TYPE_VIDEO:
				ms->vidst_index = stream_index;
				ms->vidst       = format->streams[stream_index];

				ret = decoder_init(&ms->viddec, codecctx, &ms->vidpq, ms->continue_read_thread);

				if (ret >= 0)
				{
				    decoder_start(&ms->viddec);

				    ms->video_thread     = SDL_CreateThread(viewer_video_decode_thread, "video_decoder", ms);
				    ms->check_attachment = 1;

				    if (!ms->video_thread)
				    {
					ret = -1;
					zc_log_error("Cannot create thread %s", SDL_GetError());
				    }
				}
				else zc_log_debug("Cannot init decoder");
				break;
			    default:
				break;
			}
		    }
		    else zc_log_error("No decoder could be found for codec %s", avcodec_get_name(codecctx->codec_id));

		    av_dict_free(&opts);
		}
		else
		{
		    ret = -1;
		    zc_log_error("Couldn't open codec");
		}
	    }
	    else zc_log_error("Cannot read codec parameters");
	}
	else ret = AVERROR(ENOMEM);

	if (ret < 0) avcodec_free_context(&codecctx);
    }
    else zc_log_debug("Invalid stream index");

    return ret;
}

static void viewer_stream_close(MediaState* ms, int stream_index)
{
    AVFormatContext*   format = ms->format;
    AVCodecParameters* codecpar;

    if (stream_index < 0 || stream_index >= format->nb_streams) return;
    codecpar = format->streams[stream_index]->codecpar;

    switch (codecpar->codec_type)
    {
	case AVMEDIA_TYPE_VIDEO:

	    decoder_abort(&ms->viddec, &ms->vidfq);

	    // can't we do this before decoder abort?
	    SDL_WaitThread(ms->video_thread, NULL);
	    ms->video_thread = NULL;

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

static int viewer_decode_interrupt_cb(void* ctx)
{
    MediaState* ms = ctx;
    return ms->abort_request;
}

/* this thread gets the stream from the disk or the network */
static int viewer_read_thread(void* arg)
{
    zc_log_debug("Read thread started.");

    MediaState* ms = arg;
    int         videost;
    SDL_mutex*  wait_mutex = SDL_CreateMutex();

    if (wait_mutex)
    {
	AVPacket* pkt = av_packet_alloc();

	if (pkt)
	{
	    AVFormatContext* format = avformat_alloc_context();

	    if (format)
	    {
		format->interrupt_callback.callback = viewer_decode_interrupt_cb;
		format->interrupt_callback.opaque   = ms;

		/* in case of mpeg-2 force scan all program mapping tables and combine them */
		AVDictionary* format_optsn = NULL;
		av_dict_set(&format_optsn, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);

		int ret = avformat_open_input(&format, ms->filename, NULL, &format_optsn);

		if (ret >= 0)
		{
		    ms->format = format;

		    /* print format info, should be done by coder and shown in preview */
		    av_dump_format(format, 0, ms->filename, 0);

		    /* reset eof reached, fix? */
		    if (format->pb) format->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end

		    /* get max frame duration for checking timestamp discontinuity */
		    ms->max_frame_duration = (format->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

		    /* find video streaam */
		    videost = av_find_best_stream(
			format,
			AVMEDIA_TYPE_VIDEO,
			-1,
			-1,
			NULL,
			0);

		    /* open streams */
		    int ret = -1;
		    if (videost >= 0)
		    {
			ret = viewer_stream_open(ms, videost);
			if (ret < 0) zc_log_error("Can't open video stream, errno %i", ret);
		    }

		    /* read packets forever */
		    for (;;)
		    {
			/* abort if needed */
			if (ms->abort_request) break;

			/* pause */
			if (ms->paused != ms->last_paused)
			{
			    ms->last_paused = ms->paused;
			    if (ms->paused) ms->read_pause_return = av_read_pause(format);
			    else av_read_play(format);
			}

			/* seek */
			if (ms->seek_req)
			{
			    int64_t seek_target = ms->seek_pos;
			    int64_t seek_min    = ms->seek_rel > 0 ? seek_target - ms->seek_rel + 2 : INT64_MIN;
			    int64_t seek_max    = ms->seek_rel < 0 ? seek_target - ms->seek_rel - 2 : INT64_MAX;
			    // FIXME the +-2 is due to rounding being not done in the correct direction in generation
			    //      of the seek_pos/seek_rel variables

			    /* request seek */
			    ret = avformat_seek_file(ms->format, -1, seek_min, seek_target, seek_max, ms->seek_flags);
			    if (ret < 0)
			    {
				av_log(NULL, AV_LOG_ERROR, "%s: error while seeking\n", ms->format->url);
			    }
			    else
			    {
				/* reset packet queue  */
				if (ms->vidst_index >= 0) packet_queue_flush(&ms->vidpq);

				/* set clocks */
				if (ms->seek_flags & AVSEEK_FLAG_BYTE)
				{
				    clock_set(&ms->extclk, NAN, 0);
				}
				else
				{
				    clock_set(&ms->extclk, seek_target / (double) AV_TIME_BASE, 0);
				}
			    }

			    /* reset flags */
			    ms->seek_req         = 0;
			    ms->check_attachment = 1;
			    ms->eof              = 0;

			    if (ms->paused) viewer_step_to_next_frame(ms);
			}

			/* put attachment */
			if (ms->check_attachment)
			{
			    if (ms->vidst && ms->vidst->disposition & AV_DISPOSITION_ATTACHED_PIC)
			    {
				ret = av_packet_ref(pkt, &ms->vidst->attached_pic);
				if (ret >= 0)
				{
				    /* put attached pic into packet queue */
				    packet_queue_put(&ms->vidpq, pkt);
				    packet_queue_put_nullpacket(&ms->vidpq, pkt, ms->vidst_index);
				}
				else zc_log_error("cannot ref packet");
			    }
			    ms->check_attachment = 0;
			}

			/* skip packet reading if there are enough packets */

			int full    = ms->vidpq.size > MAX_QUEUE_SIZE;
			int abort   = ms->vidpq.abort_request;
			int invalid = ms->vidst_index < 0;
			int attach  = ms->vidst->disposition & AV_DISPOSITION_ATTACHED_PIC;
			int enough  = ms->vidpq.nb_packets > (MIN_FRAMES && (!ms->vidpq.duration || av_q2d(ms->vidst->time_base) * ms->vidpq.duration > 1.0));

			if (full || invalid || abort || attach || enough)
			{
			    SDL_LockMutex(wait_mutex);
			    SDL_CondWaitTimeout(ms->continue_read_thread, wait_mutex, 10);
			    SDL_UnlockMutex(wait_mutex);
			    continue;
			}

			/* jump to start if there are no more packets left */

			int paused = ms->paused;
			int vidend = !ms->vidst || (ms->viddec.finished == ms->vidpq.serial && frame_queue_nb_remaining(&ms->vidfq) == 0);

			if (!paused && vidend)
			{
			    viewer_stream_seek(ms, 0, 0, 0);
			}

			/* read next packet */

			ret = av_read_frame(format, pkt);

			if (ret < 0)
			{
			    /* end of file reached */
			    if ((ret == AVERROR_EOF || avio_feof(format->pb)) && !ms->eof)
			    {
				if (ms->vidst_index >= 0) packet_queue_put_nullpacket(&ms->vidpq, pkt, ms->vidst_index);
				ms->eof = 1;
			    }

			    /* read error in io context */
			    if (format->pb && format->pb->error)
			    {
				break;
			    }

			    /* wait 10 ms */
			    SDL_LockMutex(wait_mutex);
			    SDL_CondWaitTimeout(ms->continue_read_thread, wait_mutex, 10);
			    SDL_UnlockMutex(wait_mutex);
			    continue;
			}
			else
			{
			    /* reset end of file */
			    ms->eof = 0;
			}

			/* check if packet is in play range specified by user, then queue, otherwise discard */

			int index_ok      = pkt->stream_index == ms->vidst_index;
			int is_attachment = ms->vidst->disposition & AV_DISPOSITION_ATTACHED_PIC;

			if (index_ok && !is_attachment)
			{
			    /* put packet in queue */
			    packet_queue_put(&ms->vidpq, pkt);
			}
			else
			{
			    /* we don't need the packet */
			    av_packet_unref(pkt);
			}
		    }
		}
		else zc_log_debug("Cannot open input, errno : %i", ret);
	    }
	    else zc_log_debug("Cannot create format context");
	}
	else zc_log_debug("Cannot create packet");
    }
    else zc_log_debug("Cannot create mutex %s", SDL_GetError());

    return 0;
}

/* entry point, opens/plays media under path */

void* viewer_open(char* path)
{
    zc_log_debug("viewer_open %s", path);

    MediaState* ms = av_mallocz(sizeof(MediaState));

    if (ms)
    {
	if (frame_queue_init(&ms->vidfq, VIDEO_PICTURE_QUEUE_SIZE, 1) >= 0)
	{
	    if (packet_queue_init(&ms->vidpq) >= 0)
	    {
		ms->continue_read_thread = SDL_CreateCond();

		if (ms->continue_read_thread)
		{
		    clock_init(&ms->vidclk, &ms->vidpq.serial);
		    clock_init(&ms->extclk, &ms->extclk.serial);

		    ms->filename    = av_strdup(path);
		    ms->read_thread = SDL_CreateThread(viewer_read_thread, "read_thread", ms);

		    if (!ms->read_thread) zc_log_error("Cannot create read thread : %s\n", SDL_GetError());
		}
		else zc_log_error("Cannot create conditional");
	    }
	    else zc_log_error("Cannot create packet queue.");
	}
	else zc_log_error("Cannot create frame queue.");
    }

    return ms;
}

/* end playing */

static void viewer_close(MediaState* ms)
{
    zc_log_debug("viewer_close %s", ms->filename);

    ms->abort_request = 1;
    SDL_WaitThread(ms->read_thread, NULL);

    /* close each stream */
    if (ms->vidst >= 0) viewer_stream_close(ms, ms->vidst_index);

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
    if (viewer_get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER)
    {
	/* if video is slave, we try to correct big delays by
	   duplicating or deleting a frame */
	diff = clock_get(&ms->vidclk) - viewer_get_master_clock(ms);

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

    // zc_log_debug("upload texture frame %i %i bitmap %i %i", frame->width, frame->height, bm->w, bm->h);

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

	    if (lastvp->serial != vp->serial) ms->frame_timer = av_gettime_relative() / 1000000.0;

	    if (ms->paused) goto display;

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
		if (!ms->step_frame && (framedrop > 0 || (framedrop && viewer_get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER)) && time > ms->frame_timer + duration)
		{
		    ms->viddec.frame_drops_late++;
		    frame_queue_next(&ms->vidfq);
		    goto retry;
		}
	    }

	    frame_queue_next(&ms->vidfq);
	    ms->force_refresh = 1;

	    if (ms->step_frame && !ms->paused) viewer_stream_toggle_pause(ms);
	}
    display:
	/* display picture */
	if (ms->force_refresh && ms->vidfq.rindex_shown) video_display(ms, bm);
    }
    ms->force_refresh = 0;
}

#endif

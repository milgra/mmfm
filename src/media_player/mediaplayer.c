/*
  Media Player namespace

  player_open creates a read thread that reads up packets from the given media.
  The read thread creates one or multiple decoder threads depending on the media streams. Decoders decode packets to frames.
  After the threads are started and reading/decoding is running a final frame can be requested with video_refresh, if there is
  a frame available, it gets copied to the given bitmap.
  Sound streams are played by SDL audio.

 */

#ifndef mediaplayer_h
#define mediaplayer_h

#include "clock.c"
#include "decoder.c"
#include "framequeue.c"
#include "ku_bitmap.c"
#include "ku_draw.c"
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"
#include "libavformat/avformat.h"
#include "libavutil/fifo.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "packetqueue.c"
#include "zc_log.c"
#include "zc_vec2.c"
#include <SDL.h>
#include <SDL_thread.h>

/* For Frequence/RDFT visualizer */
/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define SAMPLE_ARRAY_SIZE (8 * 65536)

enum av_sync_type_t
{
    AV_SYNC_AUDIO_MASTER, /* default choice */
    AV_SYNC_VIDEO_MASTER,
    AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct MediaState_t MediaState_t;

enum ms_event_id
{
    MS_EVENT_SIZE
};

typedef struct _ms_event_t
{
    enum ms_event_id id;
    MediaState_t*    ms;
    v2_t             rect;
} ms_event_t;

typedef struct AudioParams
{
    int                 freq;
    AVChannelLayout     ch_layout;
    enum AVSampleFormat fmt;
    int                 frame_size;
    int                 bytes_per_sec;
} AudioParams;

struct MediaState_t
{
    void (*on_event)(ms_event_t);
    float               duration;
    int                 finished; // playing reached end
    enum av_sync_type_t av_sync_type;
    char*               filename;             // filename, to use in functions and for format dumping
    AVFormatContext*    format;               // format context, used for aspect ratio, framerate, seek calcualtions
    SDL_Thread*         read_thread;          // thread for reading packets
    SDL_cond*           continue_read_thread; // conditional for read thread start/stop

    SDL_Thread* video_thread; // thread for video decoding
    SDL_Thread* audio_thread; // thread for video decoding

    Clock vidclk; // clock for video
    Clock audclk; // clock for audio
    Clock extclk; // clock for external

    FrameQueue  vidfq; // frame queue for video, contains complete frames
    PacketQueue vidpq; // packet queue for video, packets contain compressed data, one frame per packet in case of video

    FrameQueue  audfq; // frame queue for audio, contains uncompressed samples
    PacketQueue audpq; // packet queue for audio, packets contain compressed data, one sample per packet in case of audio

    Decoder viddec; // video decoder
    Decoder auddec; // audio decoder

    AVStream* vidst;       // video stream
    int       vidst_index; // video stream index
    AVStream* audst;       // audio stream
    int       audst_index; // audio stream index

    int wth; // lasat frame width
    int hth; // last frame height

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

    // audio related

    double             audio_clock;
    int                audio_clock_serial;
    double             audio_diff_cum; /* used for AV difference average computation */
    double             audio_diff_avg_coef;
    double             audio_diff_threshold;
    int                audio_diff_avg_count;
    int                audio_hw_buf_size;
    uint8_t*           audio_buf;
    uint8_t*           audio_buf1;
    unsigned int       audio_buf_size; /* in bytes */
    unsigned int       audio_buf1_size;
    int                audio_buf_index; /* in bytes */
    int                audio_write_buf_size;
    int                audio_volume;
    int                muted;
    struct AudioParams audio_src;
    struct AudioParams audio_tgt;

    struct SwsContext* img_convert_ctx; // image conversion context for scaling on upload
    struct SwrContext* swr_ctx;         // resample context for audio

    /* frequency/rdft visu related ( real discrete fourier transform ) */

    int64_t audio_callback_time; // for storing last audio callback

    int16_t sample_array[SAMPLE_ARRAY_SIZE];
    int     sample_array_index;
    int     last_i_start;

    RDFTContext* rdft;
    int          rdft_bits;
    FFTSample*   rdft_data;

    int xpos;
    int visutype;
};

MediaState_t* mp_open(char* path, void (*on_event)(ms_event_t event));
void          mp_play(MediaState_t* ms);
void          mp_pause(MediaState_t* ms);
void          mp_close(MediaState_t* ms);
void          mp_mute(MediaState_t* ms);
void          mp_unmute(MediaState_t* ms);
void          mp_set_volume(MediaState_t* ms, float volume);
void          mp_set_position(MediaState_t* ms, float ratio);
void          mp_set_visutype(MediaState_t* ms, int visutype);
void          mp_video_refresh(MediaState_t* opaque, double* remaining_time, ku_bitmap_t* bm, int force);
void          mp_audio_refresh(MediaState_t* opaque, ku_bitmap_t* bml, ku_bitmap_t* bmr);
double        mp_get_master_clock(MediaState_t* ms);

#endif

#if __INCLUDE_LEVEL__ == 0

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SUBPICTURE_QUEUE_SIZE 16
#define SAMPLE_QUEUE_SIZE 9
#define MIN_FRAMES 25

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
/* Step size for volume control in dB */
#define SDL_VOLUME_STEP (0.75)
/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB 20
/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD 10.0

static int framedrop = -1; // drop frames on slow cpu

double                   rdftspeed1 = 0.02;
uint8_t*                 scaledpixels[1];
static SDL_AudioDeviceID audio_dev;

// timing related

static int mp_get_master_sync_type(MediaState_t* ms)
{
    if (ms->av_sync_type == AV_SYNC_VIDEO_MASTER)
    {
	if (ms->vidst)
	    return AV_SYNC_VIDEO_MASTER;
	else
	    return AV_SYNC_AUDIO_MASTER;
    }
    else if (ms->av_sync_type == AV_SYNC_AUDIO_MASTER)
    {
	if (ms->audst)
	    return AV_SYNC_AUDIO_MASTER;
	else
	    return AV_SYNC_EXTERNAL_CLOCK;
    }
    else
    {
	return AV_SYNC_EXTERNAL_CLOCK;
    }
}

double mp_get_master_clock(MediaState_t* ms)
{
    double val;

    switch (mp_get_master_sync_type(ms))
    {
	case AV_SYNC_VIDEO_MASTER:
	    val = clock_get(&ms->vidclk);
	    break;
	case AV_SYNC_AUDIO_MASTER:
	    val = clock_get(&ms->audclk);
	    break;
	default:
	    val = clock_get(&ms->extclk);
	    break;
    }
    return val;
}

void mp_sync_clock_to_slave(Clock* c, Clock* slave)
{
    double clock       = clock_get(c);
    double slave_clock = clock_get(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
	clock_set(c, slave_clock, slave->serial);
}

// stream related

void mp_stream_seek(MediaState_t* ms, int64_t pos, int64_t rel, int by_bytes)
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

void mp_stream_toggle_pause(MediaState_t* ms)
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
    ms->paused = ms->audclk.paused = ms->vidclk.paused = ms->extclk.paused = !ms->paused;
}

void mp_step_to_next_frame(MediaState_t* ms)
{
    /* if the stream is paused unpause it, then step */
    if (ms->paused) mp_stream_toggle_pause(ms);
    ms->step_frame = 1;
}

int mp_video_decode_thread(void* arg)
{
    zc_log_debug("Video decoder thread started.");

    MediaState_t* ms    = arg;
    AVFrame*      frame = av_frame_alloc();
    int           ret   = -1;

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
		if (framedrop > 0 || (framedrop && mp_get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER))
		{
		    if (frame->pts != AV_NOPTS_VALUE)
		    {
			double diff = dpts - mp_get_master_clock(ms);
			if (!isnan(diff) && fabs(diff) > AV_NOSYNC_THRESHOLD &&
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

		// printf("frame_type=%c pts=%0.3f\n", av_get_picture_type_char(frame->pict_type), pts);

		// push to queue
		Frame* qframe = frame_queue_peek_writable(&ms->vidfq, &ms->vidpq.abort_request);

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
		    if (ms->wth != frame->width || ms->hth != frame->height)
		    {
			ms->wth          = frame->width;
			ms->hth          = frame->height;
			v2_t       rect  = (v2_t){ms->wth, ms->hth};
			ms_event_t event = {.id = MS_EVENT_SIZE, .ms = ms, .rect = rect};
			if (ms->on_event) (*ms->on_event)(event);
		    }

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

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
static int mp_synchronize_audio(MediaState_t* is, int nb_samples)
{
    int wanted_nb_samples = nb_samples;

    /* if not master, then we try to remove or add samples to correct the clock */
    if (mp_get_master_sync_type(is) != AV_SYNC_AUDIO_MASTER)
    {
	double diff, avg_diff;
	int    min_nb_samples, max_nb_samples;

	diff = clock_get(&is->audclk) - mp_get_master_clock(is);

	if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD)
	{
	    is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
	    if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB)
	    {
		/* not enough measures to have a correct estimate */
		is->audio_diff_avg_count++;
	    }
	    else
	    {
		/* estimate the A-V difference */
		avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

		if (fabs(avg_diff) >= is->audio_diff_threshold)
		{
		    wanted_nb_samples = nb_samples + (int) (diff * is->audio_src.freq);
		    min_nb_samples    = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
		    max_nb_samples    = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
		    wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
		}
		av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n", diff, avg_diff, wanted_nb_samples - nb_samples, is->audio_clock, is->audio_diff_threshold);
	    }
	}
	else
	{
	    /* too big difference : may be initial PTS errors, so
	       reset A-V filter */
	    is->audio_diff_avg_count = 0;
	    is->audio_diff_cum       = 0;
	}
    }

    return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
int mp_audio_decode_frame(MediaState_t* is)
{
    int              data_size, resampled_data_size;
    av_unused double audio_clock0;
    int              wanted_nb_samples;
    Frame*           af;

    if (is->paused)
	return -1;

    do {
	if (!(af = frame_queue_peek_readable(&is->audfq, &is->audpq.abort_request)))
	    return -1;
	frame_queue_next(&is->audfq);
    } while (af->serial != is->audpq.serial);

    data_size = av_samples_get_buffer_size(NULL, af->frame->ch_layout.nb_channels, af->frame->nb_samples, af->frame->format, 1);

    wanted_nb_samples = mp_synchronize_audio(is, af->frame->nb_samples);

    if (af->frame->format != is->audio_src.fmt ||
	av_channel_layout_compare(&af->frame->ch_layout, &is->audio_src.ch_layout) ||
	af->frame->sample_rate != is->audio_src.freq ||
	(wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx))
    {
	swr_free(&is->swr_ctx);
	swr_alloc_set_opts2(&is->swr_ctx, &is->audio_tgt.ch_layout, is->audio_tgt.fmt, is->audio_tgt.freq, &af->frame->ch_layout, af->frame->format, af->frame->sample_rate, 0, NULL);
	if (!is->swr_ctx || swr_init(is->swr_ctx) < 0)
	{
	    av_log(NULL, AV_LOG_ERROR, "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n", af->frame->sample_rate, av_get_sample_fmt_name(af->frame->format), af->frame->ch_layout.nb_channels, is->audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.ch_layout.nb_channels);
	    swr_free(&is->swr_ctx);
	    return -1;
	}
	if (av_channel_layout_copy(&is->audio_src.ch_layout, &af->frame->ch_layout) < 0)
	    return -1;
	is->audio_src.freq = af->frame->sample_rate;
	is->audio_src.fmt  = af->frame->format;
    }

    if (is->swr_ctx)
    {
	const uint8_t** in        = (const uint8_t**) af->frame->extended_data;
	uint8_t**       out       = &is->audio_buf1;
	int             out_count = (int64_t) wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
	int             out_size  = av_samples_get_buffer_size(NULL, is->audio_tgt.ch_layout.nb_channels, out_count, is->audio_tgt.fmt, 0);
	int             len2;
	if (out_size < 0)
	{
	    av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
	    return -1;
	}
	if (wanted_nb_samples != af->frame->nb_samples)
	{
	    if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) * is->audio_tgt.freq / af->frame->sample_rate, wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) < 0)
	    {
		av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
		return -1;
	    }
	}
	av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
	if (!is->audio_buf1)
	    return AVERROR(ENOMEM);
	len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
	if (len2 < 0)
	{
	    av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
	    return -1;
	}
	if (len2 == out_count)
	{
	    av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
	    if (swr_init(is->swr_ctx) < 0)
		swr_free(&is->swr_ctx);
	}
	is->audio_buf       = is->audio_buf1;
	resampled_data_size = len2 * is->audio_tgt.ch_layout.nb_channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
    }
    else
    {
	is->audio_buf       = af->frame->data[0];
	resampled_data_size = data_size;
    }

    audio_clock0 = is->audio_clock;
    /* update the audio clock with the pts */
    if (!isnan(af->pts))
	is->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    else
	is->audio_clock = NAN;
    is->audio_clock_serial = af->serial;
#ifdef DEBUG
    {
	static double last_clock;
	// printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n", is->audio_clock - last_clock, is->audio_clock, audio_clock0);
	last_clock = is->audio_clock;
    }
#endif
    return resampled_data_size;
}

/* copy samples for viewing in editor window */
static void update_sample_display(MediaState_t* is, short* samples, int samples_size)
{
    int size, len;

    size = samples_size / sizeof(short);
    while (size > 0)
    {
	len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
	if (len > size)
	    len = size;
	memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
	samples += len;
	is->sample_array_index += len;
	if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
	    is->sample_array_index = 0;
	size -= len;
    }
}

/* prepare a new audio buffer */
static void mp_sdl_audio_callback(void* opaque, Uint8* stream, int len)
{
    MediaState_t* ms = opaque;
    int           audio_size, len1;

    ms->audio_callback_time = av_gettime_relative();

    while (len > 0)
    {
	if (ms->audio_buf_index >= ms->audio_buf_size)
	{
	    audio_size = mp_audio_decode_frame(ms);

	    if (audio_size < 0)
	    {
		/* if error, just output silence */
		ms->audio_buf      = NULL;
		ms->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / ms->audio_tgt.frame_size * ms->audio_tgt.frame_size;
	    }
	    else
	    {
		// TODO use this in oscilloscope
		update_sample_display(ms, (int16_t*) ms->audio_buf, audio_size);

		ms->audio_buf_size = audio_size;
	    }
	    ms->audio_buf_index = 0;
	}
	len1 = ms->audio_buf_size - ms->audio_buf_index;
	if (len1 > len)
	    len1 = len;

	if (!ms->muted && ms->audio_buf && ms->audio_volume == SDL_MIX_MAXVOLUME)
	{
	    memcpy(stream, (uint8_t*) ms->audio_buf + ms->audio_buf_index, len1);
	}
	else
	{
	    memset(stream, 0, len1);
	    if (!ms->muted && ms->audio_buf)
		SDL_MixAudioFormat(stream, (uint8_t*) ms->audio_buf + ms->audio_buf_index, AUDIO_S16SYS, len1, ms->audio_volume);
	}
	len -= len1;
	stream += len1;
	ms->audio_buf_index += len1;
    }
    ms->audio_write_buf_size = ms->audio_buf_size - ms->audio_buf_index;

    /* Let's assume the audio driver that ms used by SDL has two periods. */
    if (!isnan(ms->audio_clock))
    {
	clock_set_at(&ms->audclk, ms->audio_clock - (double) (2 * ms->audio_hw_buf_size + ms->audio_write_buf_size) / ms->audio_tgt.bytes_per_sec, ms->audio_clock_serial, ms->audio_callback_time / 1000000.0);
	mp_sync_clock_to_slave(&ms->extclk, &ms->audclk);
    }
}

int mp_audio_open(void* opaque, AVChannelLayout* wanted_channel_layout, int wanted_sample_rate, struct AudioParams* audio_hw_params)
{
    zc_log_debug("viewer audio open");

    SDL_AudioSpec    wanted_spec, spec;
    const char*      env;
    static const int next_nb_channels[]   = {0, 0, 1, 6, 2, 6, 4, 6};
    static const int next_sample_rates[]  = {0, 44100, 48000, 96000, 192000};
    int              next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;
    int              wanted_nb_channels   = wanted_channel_layout->nb_channels;

    env = SDL_getenv("SDL_AUDIO_CHANNELS");

    zc_log_debug("ENV CHANNELS %s", env);

    if (env)
    {
	wanted_nb_channels = atoi(env);
	av_channel_layout_uninit(wanted_channel_layout);
	av_channel_layout_default(wanted_channel_layout, wanted_nb_channels);
    }

    if (wanted_channel_layout->order != AV_CHANNEL_ORDER_NATIVE)
    {
	av_channel_layout_uninit(wanted_channel_layout);
	av_channel_layout_default(wanted_channel_layout, wanted_nb_channels);
    }

    wanted_nb_channels   = wanted_channel_layout->nb_channels;
    wanted_spec.channels = wanted_nb_channels;
    wanted_spec.freq     = wanted_sample_rate;

    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0)
    {
	av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
	return -1;
    }
    while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
	next_sample_rate_idx--;

    wanted_spec.format   = AUDIO_S16SYS;
    wanted_spec.silence  = 0;
    wanted_spec.samples  = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    wanted_spec.callback = mp_sdl_audio_callback;
    wanted_spec.userdata = opaque;

    while (!(audio_dev = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &spec, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE)))
    {
	av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n", wanted_spec.channels, wanted_spec.freq, SDL_GetError());
	wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
	if (!wanted_spec.channels)
	{
	    wanted_spec.freq     = next_sample_rates[next_sample_rate_idx--];
	    wanted_spec.channels = wanted_nb_channels;
	    if (!wanted_spec.freq)
	    {
		av_log(NULL, AV_LOG_ERROR, "No more combinations to try, audio open failed\n");
		return -1;
	    }
	}
	av_channel_layout_default(wanted_channel_layout, wanted_spec.channels);
    }

    if (spec.format != AUDIO_S16SYS)
    {
	av_log(NULL, AV_LOG_ERROR, "SDL advised audio format %d is not supported!\n", spec.format);
	return -1;
    }

    if (spec.channels != wanted_spec.channels)
    {
	av_channel_layout_uninit(wanted_channel_layout);
	av_channel_layout_default(wanted_channel_layout, spec.channels);
	if (wanted_channel_layout->order != AV_CHANNEL_ORDER_NATIVE)
	{
	    av_log(NULL, AV_LOG_ERROR, "SDL advised channel count %d is not supported!\n", spec.channels);
	    return -1;
	}
    }

    audio_hw_params->fmt  = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = spec.freq;

    if (av_channel_layout_copy(&audio_hw_params->ch_layout, wanted_channel_layout) < 0)
	return -1;

    audio_hw_params->frame_size    = av_samples_get_buffer_size(NULL, audio_hw_params->ch_layout.nb_channels, 1, audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->ch_layout.nb_channels, audio_hw_params->freq, audio_hw_params->fmt, 1);

    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0)
    {
	av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
	return -1;
    }

    zc_log_debug("spec size %i channels %i freq %i", spec.size, audio_hw_params->ch_layout.nb_channels, spec.freq);

    return spec.size;
}

int mp_audio_decode_thread(void* arg)
{
    zc_log_debug("Audio decoder thread started.");

    MediaState_t* is    = arg;
    AVFrame*      frame = av_frame_alloc();
    int           ret   = 0;

    if (frame)
    {
	do {
	    int got_frame = decoder_decode_frame(&is->auddec, frame);

	    if (got_frame >= 0)
	    {
		if (got_frame)
		{
		    AVRational tb = (AVRational){1, frame->sample_rate};
		    Frame*     af = frame_queue_peek_writable(&is->audfq, &is->audpq.abort_request);

		    if (af)
		    {
			af->pts      = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);
			af->pos      = frame->pkt_pos;
			af->serial   = is->auddec.pkt_serial;
			af->duration = av_q2d((AVRational){frame->nb_samples, frame->sample_rate});

			av_frame_move_ref(af->frame, frame);
			frame_queue_push(&is->audfq);
		    }
		    else break;
		}
	    }
	    else break;

	} while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);

	av_frame_free(&frame); // cleanup
    }
    else
    {
	ret = AVERROR(ENOMEM); // cannot allocate frame
    }

    zc_log_debug("Audio decoder thread finished.");

    return ret;
}

int mp_stream_open(MediaState_t* ms, int stream_index)
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

				    ms->video_thread     = SDL_CreateThread(mp_video_decode_thread, "video_decoder", ms);
				    ms->check_attachment = 1;

				    if (!ms->video_thread)
				    {
					ret = -1;
					zc_log_error("Cannot create thread %s", SDL_GetError());
				    }
				}
				else zc_log_error("Cannot init decoder");
				break;

			    case AVMEDIA_TYPE_AUDIO:
			    {
				AVChannelLayout ch_layout   = {0};
				int             sample_rate = codecctx->sample_rate;
				ret                         = av_channel_layout_copy(&ch_layout, &codecctx->ch_layout);

				if (ret >= 0)
				{
				    /* prepare audio output */
				    ret = mp_audio_open(ms, &ch_layout, sample_rate, &ms->audio_tgt);

				    if (ret >= 0)
				    {
					ms->audio_hw_buf_size = ret;
					ms->audio_src         = ms->audio_tgt;
					ms->audio_buf_size    = 0;
					ms->audio_buf_index   = 0;

					/* init averaging filter */
					ms->audio_diff_avg_coef  = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
					ms->audio_diff_avg_count = 0;

					/* since we do not have a precise anough audio FIFO fullness,
					   we correct audio sync only if larger than this threshold */
					ms->audio_diff_threshold = (double) (ms->audio_hw_buf_size) / ms->audio_tgt.bytes_per_sec;

					ms->audst_index = stream_index;
					ms->audst       = format->streams[stream_index];

					ret = decoder_init(&ms->auddec, codecctx, &ms->audpq, ms->continue_read_thread);

					if (ret >= 0)
					{
					    if ((ms->format->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) && !ms->format->iformat->read_seek)
					    {
						ms->auddec.start_pts    = ms->audst->start_time;
						ms->auddec.start_pts_tb = ms->audst->time_base;
					    }

					    decoder_start(&ms->auddec);

					    ms->audio_thread = SDL_CreateThread(mp_audio_decode_thread, "audio_decoder", ms);

					    if (!ms->audio_thread)
					    {
						ret = -1;
						zc_log_error("Cannot create thread %s", SDL_GetError());
					    }

					    SDL_PauseAudioDevice(audio_dev, 0);
					}
					else zc_log_error("Cannot init decoder");
				    }
				    else zc_log_error("Cannot open audio output");
				}
				else zc_log_error("Cannot copy channel layout");

				break;
			    }
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

void mp_stream_close(MediaState_t* ms, int stream_index)
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

	case AVMEDIA_TYPE_AUDIO:

	    decoder_abort(&ms->auddec, &ms->audfq);

	    // can't we do this before decoder abort?
	    SDL_WaitThread(ms->audio_thread, NULL);
	    ms->audio_thread = NULL;

	    SDL_CloseAudioDevice(audio_dev);
	    decoder_destroy(&ms->auddec);

	    swr_free(&ms->swr_ctx);
	    av_freep(&ms->audio_buf1);

	    ms->audio_buf1_size = 0;
	    ms->audio_buf       = NULL;

	    /* if (ms->rdft) */
	    /* { */
	    /* 	av_rdft_end(ms->rdft); */
	    /* 	av_freep(&ms->rdft_data); */
	    /* 	ms->rdft      = NULL; */
	    /* 	ms->rdft_bits = 0; */
	    /* } */
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
	case AVMEDIA_TYPE_AUDIO:
	    ms->audst       = NULL;
	    ms->audst_index = -1;
	    break;
	default:
	    break;
    }
}

int mp_decode_interrupt_cb(void* ctx)
{
    MediaState_t* ms = ctx;
    return ms->abort_request;
}

int mp_stream_has_enough_packets(AVStream* st, int stream_id, PacketQueue* queue)
{
    return stream_id < 0 ||
	   queue->abort_request ||
	   (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
	   ((queue->nb_packets > MIN_FRAMES) && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0));
}

/* this thread gets the stream from the disk or the network */
int mp_read_thread(void* arg)
{
    zc_log_debug("Read thread started.");

    MediaState_t* ms = arg;
    int           videost;
    int           audiost;
    SDL_mutex*    wait_mutex = SDL_CreateMutex();

    if (wait_mutex)
    {
	AVPacket* pkt = av_packet_alloc();

	if (pkt)
	{
	    AVFormatContext* format = avformat_alloc_context();

	    if (format)
	    {
		format->interrupt_callback.callback = mp_decode_interrupt_cb;
		format->interrupt_callback.opaque   = ms;

		/* in case of mpeg-2 force scan all program mapping tables and combine them */
		AVDictionary* format_opts       = NULL;
		int           scan_all_pmts_set = 0;

		if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE))
		{
		    av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
		    scan_all_pmts_set = 1;
		}

		int ret = avformat_open_input(&format, ms->filename, NULL, &format_opts);

		if (ret >= 0)
		{
		    ms->format = format;

		    /* I don't know yet why we have to do this */
		    if (scan_all_pmts_set) av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

		    /* why? */
		    av_format_inject_global_side_data(format);

		    /* find stream info before dump */
		    ret = avformat_find_stream_info(format, NULL);
		    if (ret < 0) zc_log_debug("can't find stream info");

		    /* print format info, should be done by coder and shown in preview */
		    av_dump_format(format, 0, ms->filename, 0);

		    /* reset eof reached, fix? */
		    if (format->pb) format->pb->eof_reached = 0; // FIXME hack, ffplay maybe should not use avio_feof() to test for the end http://www.gidnetwork.com/b-58.html

		    /* get max frame duration for checking timestamp discontinuity */
		    ms->max_frame_duration = (format->iformat->flags & AVFMT_TS_DISCONT) ? 10.0 : 3600.0;

		    ms->duration = (float) format->duration / (float) AV_TIME_BASE;

		    /* find video streaam */
		    videost = av_find_best_stream(
			format,
			AVMEDIA_TYPE_VIDEO,
			-1,
			-1,
			NULL,
			0);

		    /* find audio stream */
		    audiost = av_find_best_stream(
			format,
			AVMEDIA_TYPE_AUDIO,
			-1,
			videost,
			NULL,
			0);

		    /* open streams */
		    int ret = -1;
		    if (videost >= 0)
		    {
			ret = mp_stream_open(ms, videost);
			if (ret < 0) zc_log_error("Can't open video stream, errno %i", ret);
		    }
		    else zc_log_debug("No video stream.");

		    if (audiost >= 0)
		    {
			ret = mp_stream_open(ms, audiost);
			if (ret < 0) zc_log_error("Can't open audio stream, errno %i", ret);
		    }
		    else zc_log_debug("No audio stream.");

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

			/* seek if requested */
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
				/* reset packet queues  */
				if (ms->audst_index >= 0) packet_queue_flush(&ms->audpq);
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

			    if (ms->paused) mp_step_to_next_frame(ms);
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

			if ((ms->vidpq.size + ms->audpq.size > MAX_QUEUE_SIZE) ||
			    (mp_stream_has_enough_packets(ms->audst, ms->audst_index, &ms->audpq) &&
			     mp_stream_has_enough_packets(ms->vidst, ms->vidst_index, &ms->vidpq)))
			{
			    SDL_LockMutex(wait_mutex);
			    SDL_CondWaitTimeout(ms->continue_read_thread, wait_mutex, 10);
			    SDL_UnlockMutex(wait_mutex);
			    continue;
			}

			/* jump to start if there are no more packets left */

			int paused = ms->paused;
			int audend = !ms->audst || (ms->auddec.finished == ms->audpq.serial && frame_queue_nb_remaining(&ms->audfq) == 0);
			int vidend = !ms->vidst || (ms->viddec.finished == ms->vidpq.serial && frame_queue_nb_remaining(&ms->vidfq) == 0);

			if (!paused && vidend && audend)
			{
			    mp_stream_seek(ms, 0, 0, 0);
			    ms->finished = 1;
			}

			/* read next packet */

			ret = av_read_frame(format, pkt);

			if (ret < 0)
			{
			    /* end of file reached */
			    if ((ret == AVERROR_EOF || avio_feof(format->pb)) && !ms->eof)
			    {
				if (ms->vidst_index >= 0) packet_queue_put_nullpacket(&ms->vidpq, pkt, ms->vidst_index);
				if (ms->audst_index >= 0) packet_queue_put_nullpacket(&ms->audpq, pkt, ms->audst_index);
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

			/* finally queue packet or discard */

			if (pkt->stream_index == ms->audst_index)
			{
			    packet_queue_put(&ms->audpq, pkt);
			}
			else if (pkt->stream_index == ms->vidst_index && !(ms->vidst->disposition & AV_DISPOSITION_ATTACHED_PIC))
			{
			    packet_queue_put(&ms->vidpq, pkt);
			}
			else
			{
			    av_packet_unref(pkt);
			}
		    }
		}
		else zc_log_debug("Cannot open input, errno : %i", ret);

		av_dict_free(&format_opts);
	    }
	    else zc_log_debug("Cannot create format context");

	    av_packet_free(&pkt);
	}
	else zc_log_debug("Cannot create packet");

	SDL_DestroyMutex(wait_mutex);
    }
    else zc_log_debug("Cannot create mutex %s", SDL_GetError());

    zc_log_debug("Read thread finished.");

    return 0;
}

/* entry point, opens/plays media under path */

MediaState_t* mp_open(char* path, void (*on_event)(ms_event_t event))
{
    zc_log_debug("mp_open %s", path);

    MediaState_t* ms = av_mallocz(sizeof(MediaState_t));

    if (ms)
    {
	int res1 = frame_queue_init(&ms->audfq, SAMPLE_QUEUE_SIZE, 1);
	int res2 = frame_queue_init(&ms->vidfq, VIDEO_PICTURE_QUEUE_SIZE, 1);

	if (res1 >= 0 && res2 >= 0)
	{
	    res1 = packet_queue_init(&ms->vidpq);
	    res2 = packet_queue_init(&ms->audpq);

	    if (res1 >= 0 && res2 >= 0)
	    {
		ms->continue_read_thread = SDL_CreateCond();

		if (ms->continue_read_thread)
		{
		    clock_init(&ms->vidclk, &ms->vidpq.serial);
		    clock_init(&ms->audclk, &ms->audpq.serial);
		    clock_init(&ms->extclk, &ms->extclk.serial);
		    printf("VIDCLK %zu\n", (size_t) &ms->vidclk);
		    printf("AUDCLK %zu\n", (size_t) &ms->audclk);
		    printf("EXTCLK %zu\n", (size_t) &ms->extclk);

		    ms->on_event     = on_event;
		    ms->audio_volume = 100;
		    ms->vidst_index  = -1;
		    ms->audst_index  = -1;
		    ms->filename     = av_strdup(path);
		    ms->read_thread  = SDL_CreateThread(mp_read_thread, "read_thread", ms);

		    if (!ms->read_thread) zc_log_error("Cannot create read thread : %s\n", SDL_GetError());
		}
		else zc_log_error("Cannot create conditional");
	    }
	    else zc_log_error("Cannot create packet queue.");
	}
	else zc_log_error("Cannot create frame queues.");
    }

    return ms;
}

/* end playing */

void mp_close(MediaState_t* ms)
{
    zc_log_debug("mp_close %s", ms->filename);

    ms->abort_request = 1;
    SDL_WaitThread(ms->read_thread, NULL);

    /* close each stream */
    if (ms->vidst_index >= 0) mp_stream_close(ms, ms->vidst_index);
    if (ms->audst_index >= 0) mp_stream_close(ms, ms->audst_index);

    avformat_close_input(&ms->format);

    packet_queue_destroy(&ms->vidpq);
    packet_queue_destroy(&ms->audpq);

    frame_queue_destroy(&ms->vidfq);
    frame_queue_destroy(&ms->audfq);

    SDL_DestroyCond(ms->continue_read_thread);

    sws_freeContext(ms->img_convert_ctx);

    av_free(ms->filename);
    av_free(ms);
}

void mp_play(MediaState_t* ms)
{
    if (ms->paused) mp_stream_toggle_pause(ms);
}

void mp_pause(MediaState_t* ms)
{
    if (!ms->paused) mp_stream_toggle_pause(ms);
}

void mp_mute(MediaState_t* ms)
{
    if (ms) ms->muted = 1;
}

void mp_unmute(MediaState_t* ms)
{
    if (ms) ms->muted = 0;
}

void mp_set_volume(MediaState_t* ms, float ratio)
{
    ms->audio_volume = (int) ((float) SDL_MIX_MAXVOLUME * ratio);
}

void mp_set_position(MediaState_t* ms, float ratio)
{
    int newpos = (int) ms->duration * ratio;
    int diff   = (int) mp_get_master_clock(ms) - newpos;
    /* printf("ratio %f\n", ratio); */
    /* printf("duration %f\n", player_duration()); */
    /* printf("newpos %i\n", newpos); */
    mp_stream_seek(ms, (int64_t) (newpos * AV_TIME_BASE), (int64_t) (diff * AV_TIME_BASE), 0);
}

void mp_set_visutype(MediaState_t* ms, int visutype)
{
    ms->visutype = visutype;
}

// display related

void update_video_pts(MediaState_t* ms, double pts, int64_t pos, int serial)
{
    /* update current video pts */
    clock_set(&ms->vidclk, pts, serial);
    mp_sync_clock_to_slave(&ms->extclk, &ms->vidclk);
}
double compute_target_delay(double delay, MediaState_t* ms)
{
    double sync_threshold, diff = 0;

    /* update delay to follow master synchronisation source */
    if (mp_get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER)
    {
	/* if video is slave, we try to correct big delays by
	   duplicating or deleting a frame */
	diff = clock_get(&ms->vidclk) - mp_get_master_clock(ms);

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

double vp_duration(MediaState_t* ms, Frame* vp, Frame* nextvp)
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

unsigned sws_flags = SWS_FAST_BILINEAR;

int upload_texture(SDL_Texture** tex, AVFrame* frame, struct SwsContext** img_convert_ctx, ku_bitmap_t* bm)
{
    int ret = 0;

    if (frame->width > 0 && frame->height > 0)
    {
	*img_convert_ctx = sws_getCachedContext(*img_convert_ctx, frame->width, frame->height, frame->format, bm->w, bm->h, AV_PIX_FMT_RGBA, sws_flags, NULL, NULL, NULL);

	if (*img_convert_ctx != NULL)
	{
	    scaledpixels[0] = bm->data;
	    int pitch[4];
	    pitch[0] = bm->w * 4;
	    sws_scale(*img_convert_ctx, (const uint8_t* const*) frame->data, frame->linesize, 0, frame->height, scaledpixels, pitch);
	}
    }
    else
	zc_log_debug("invalid frame");
    return ret;
}

void video_image_display(MediaState_t* ms, ku_bitmap_t* bm, int force)
{
    Frame* vp;
    // Frame*   sp = NULL;
    // SDL_Rect rect;

    vp = frame_queue_peek_last(&ms->vidfq);

    if (!vp->uploaded || force)
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
void video_display(MediaState_t* ms, ku_bitmap_t* bm, int force)
{
    if (ms->vidst) video_image_display(ms, bm, force);
}

/* called to display each frame */
void mp_video_refresh(MediaState_t* ms, double* remaining_time, ku_bitmap_t* bm, int force)
{
    double time;

    // Frame *sp, *sp2;

    time = av_gettime_relative() / 1000000.0;
    if (ms->force_refresh || ms->last_vis_time + rdftspeed1 < time)
    {
	video_display(ms, bm, force);
	ms->last_vis_time = time;
    }
    *remaining_time = FFMIN(*remaining_time, ms->last_vis_time + rdftspeed1 - time);

    if (ms->vidst)
    {
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
		if (!ms->step_frame && (framedrop > 0 || (framedrop && mp_get_master_sync_type(ms) != AV_SYNC_VIDEO_MASTER)) && time > ms->frame_timer + duration)
		{
		    ms->viddec.frame_drops_late++;
		    frame_queue_next(&ms->vidfq);
		    goto retry;
		}
	    }

	    frame_queue_next(&ms->vidfq);
	    ms->force_refresh = 1;

	    if (ms->step_frame && !ms->paused) mp_stream_toggle_pause(ms);
	}
    display:
	/* display picture */
	if (ms->force_refresh && ms->vidfq.rindex_shown) video_display(ms, bm, force);
    }
    ms->force_refresh = 0;
}

static inline int compute_mod(int a, int b)
{
    return a < 0 ? a % b + b : a % b;
}

void mp_audio_refresh(MediaState_t* ms, ku_bitmap_t* bml, ku_bitmap_t* bmr)
{
    int     i, i_start, x, y1, y2, y, ys, delay, n, nb_display_channels;
    int     ch, channels, h, h2;
    int64_t time_diff;
    int     rdft_bits, nb_freq;

    int edge   = 0;
    int width  = bml->w - 2 * edge;
    int height = bml->h - 2 * edge;

    /* prepare bits */

    for (rdft_bits = 1; (1 << rdft_bits) < 2 * height; rdft_bits++)
	;
    nb_freq = 1 << (rdft_bits - 1);

    /* /\* compute display index : center on currently output samples *\/ */
    channels = ms->audio_tgt.ch_layout.nb_channels;

    if (channels == 0) return;

    nb_display_channels = channels;

    if (!ms->paused)
    {
	int data_used = ms->visutype == 0 ? width : (2 * nb_freq);
	n             = 2 * channels;
	delay         = ms->audio_write_buf_size;

	delay /= n;

	/* to be more precise, we take into account the time spent since
	       the last buffer computation */
	if (ms->audio_callback_time)
	{
	    time_diff = av_gettime_relative() - ms->audio_callback_time;
	    delay -= (time_diff * ms->audio_tgt.freq) / 1000000;
	}

	delay += 2 * data_used;
	if (delay < data_used)
	    delay = data_used;

	i_start = x = compute_mod(ms->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);

	if (ms->visutype == 0)
	{
	    h = INT_MIN;
	    for (i = 0; i < 1000; i += channels)
	    {
		int idx   = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
		int a     = ms->sample_array[idx];
		int b     = ms->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
		int c     = ms->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
		int d     = ms->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
		int score = a - d;

		if (h < score && (b ^ c) < 0)
		{
		    h       = score;
		    i_start = idx;
		}
	    }
	}

	ms->last_i_start = i_start;
    }
    else
    {
	i_start = ms->last_i_start;
    }

    int ytop  = 0;
    int xleft = 0;

    if (ms->visutype == 0) // frequency
    {
	ku_draw_rect(bml, edge, edge, width, height, 0x000000FF, 1);
	ku_draw_rect(bmr, edge, edge, width, height, 0x000000FF, 1);

	/* total height for one channel */
	h = height;
	/* graph height / 2 */
	h2 = (h * 9) / 20;

	for (ch = 0; ch < nb_display_channels; ch++)
	{
	    i  = i_start + ch;
	    y1 = ytop + (h / 2); /* position of center line */

	    ku_bitmap_t* bitmap = ch == 0 ? bml : bmr;

	    int prevy = -1;
	    for (x = 0; x < width; x++)
	    {
		y = (ms->sample_array[i] * h2) >> 15;
		if (y < 0)
		{
		    y  = -y;
		    ys = y1 - y;
		    y2 = ys;
		}
		else
		{
		    ys = y1;
		    y2 = ys + y;
		}

		if (prevy < 0) prevy = y2;

		int sy = prevy < y2 ? prevy : y2;
		int hy = prevy < y2 ? y2 - prevy : prevy - y2;
		if (hy == 0) hy = 1;

		ku_draw_rect(bitmap, xleft + x, sy, 1, hy, 0xFFFFFFFF, 1);

		prevy = y2;

		i += channels;
		if (i >= SAMPLE_ARRAY_SIZE)
		    i -= SAMPLE_ARRAY_SIZE;
	    }
	}
    }
    else // RDFT
    {
	/* 	/\* if (realloc_texture(&ms->vis_texture, SDL_PIXELFORMAT_ARGB8888, width, height, SDL_BLENDMODE_NONE, 1) < 0) *\/ */
	/* 	/\*   return; *\/ */

	nb_display_channels = FFMIN(nb_display_channels, 2);
	if (rdft_bits != ms->rdft_bits)
	{
	    av_rdft_end(ms->rdft);
	    av_free(ms->rdft_data);
	    ms->rdft      = av_rdft_init(rdft_bits, DFT_R2C);
	    ms->rdft_bits = rdft_bits;
	    ms->rdft_data = av_malloc_array(nb_freq, 4 * sizeof(*ms->rdft_data));
	};

	if (!ms->rdft || !ms->rdft_data)
	{
	    /* 	    av_log(NULL, AV_LOG_ERROR, "Failed to allocate buffers for RDFT, switching to waves display\n"); */
	    /* 	    show_mode = 0; */
	}
	else
	{
	    FFTSample* data[2];
	    int        pitch;

	    for (int ch = 0; ch < nb_display_channels; ch++)
	    {
		data[ch] = ms->rdft_data + 2 * nb_freq * ch;
		i        = i_start + ch;
		for (x = 0; x < 2 * nb_freq; x++)
		{
		    double w    = (x - nb_freq) * (1.0 / nb_freq);
		    data[ch][x] = ms->sample_array[i] * (1.0 - w * w);
		    i += channels;
		    if (i >= SAMPLE_ARRAY_SIZE) i -= SAMPLE_ARRAY_SIZE;
		}
		av_rdft_calc(ms->rdft, data[ch]);
	    }

	    pitch = width;
	    pitch >>= 2;

	    for (y = 0; y < height; y++)
	    {
		double w = 1 / sqrt(nb_freq);
		int    l = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] + data[0][2 * y + 1] * data[0][2 * y + 1]));
		int    r = sqrt(w * sqrt(data[1][2 * y + 0] * data[1][2 * y + 0] + data[1][2 * y + 1] * data[1][2 * y + 1]));

		// int    b = (nb_display_channels == 2) ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
		// : a;
		l = FFMIN(l, 255);
		r = FFMIN(r, 255);

		uint32_t colorl = ((l << 16) + (l << 8) + l) << 8 | 0xFF;
		uint32_t colorr = ((r << 16) + (r << 8) + r) << 8 | 0xFF;

		int h = bml->h;
		ku_draw_rect(bml, ms->xpos + edge, h - edge - y, 1, 1, colorl, 1);
		ku_draw_rect(bmr, ms->xpos + edge, h - edge - y, 1, 1, colorr, 1);
	    }

	    if (ms->xpos < width - 1)
	    {
		ku_draw_rect(bml, ms->xpos + edge + 1, 1, 1, height - 1, 0xFFFFFFFF, 1);
		ku_draw_rect(bmr, ms->xpos + edge + 1, 1, 1, height - 1, 0xFFFFFFFF, 1);
	    }

	    if (!ms->paused)
		ms->xpos++;
	    if (ms->xpos >= width)
		ms->xpos = 0;
	}
    }
}

#endif

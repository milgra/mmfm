#ifndef coder_h
#define coder_h

#include "ku_bitmap.c"
#include "zc_map.c"
#include "zc_vector.c"

typedef enum _coder_media_type_t
{
    CODER_MEDIA_TYPE_VIDEO,
    CODER_MEDIA_TYPE_AUDIO,
    CODER_MEDIA_TYPE_IMAGE,
    CODER_MEDIA_TYPE_OTHER
} coder_media_type_t;

ku_bitmap_t*       coder_load_image(const char* path);
void               coder_load_image_into(const char* path, ku_bitmap_t* bitmap);
int                coder_load_cover_into(const char* path, ku_bitmap_t* bitmap);
int                coder_load_metadata_into(const char* path, map_t* map);
coder_media_type_t coder_get_type(const char* path);
int                coder_write_metadata(char* libpath, char* path, char* cover_path, map_t* data, vec_t* drop);
int                coder_write_png(char* path, ku_bitmap_t* bm);

#endif

#if __INCLUDE_LEVEL__ == 0
#include "ku_draw.c"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "zc_cstring.c"
#include "zc_log.c"
#include "zc_memory.c"
#include "zc_path.c"
#include <limits.h>

struct SwsContext* coder_sws_context = NULL;

ku_bitmap_t* coder_load_image(const char* path)
{
    ku_bitmap_t* bitmap  = NULL;
    int          success = 0;

    AVFormatContext* src_ctx = avformat_alloc_context(); // FREE 0

    /* // open the specified path */
    if (avformat_open_input(&src_ctx, path, NULL, NULL) == 0) // CLOSE 0
    {
	// find the first attached picture, if available
	for (int index = 0; index < src_ctx->nb_streams; index++)
	{
	    if (src_ctx->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	    {
		AVCodecParameters* param = src_ctx->streams[index]->codecpar;

		const AVCodec*  codec        = avcodec_find_decoder(param->codec_id);
		AVCodecContext* codecContext = avcodec_alloc_context3(codec); // FREE 1

		if (codecContext != NULL)
		{
		    avcodec_parameters_to_context(codecContext, param);
		    avcodec_open2(codecContext, codec, NULL); // CLOSE 1

		    AVPacket* packet = av_packet_alloc(); // FREE 2
		    AVFrame*  frame  = av_frame_alloc();  // FREE 3

		    while (av_read_frame(src_ctx, packet) >= 0)
		    {
			avcodec_send_packet(codecContext, packet);
		    }

		    avcodec_receive_frame(codecContext, frame);

		    enum AVPixelFormat pixFormat;
		    switch (frame->format)
		    {
			case AV_PIX_FMT_YUVJ420P:
			    pixFormat = AV_PIX_FMT_YUV420P;
			    break;
			case AV_PIX_FMT_YUVJ422P:
			    pixFormat = AV_PIX_FMT_YUV422P;
			    break;
			case AV_PIX_FMT_YUVJ444P:
			    pixFormat = AV_PIX_FMT_YUV444P;
			    break;
			case AV_PIX_FMT_YUVJ440P:
			    pixFormat = AV_PIX_FMT_YUV440P;
			    break;
			default:
			    pixFormat = frame->format;
			    break;
		    }

		    coder_sws_context = sws_getCachedContext(
			coder_sws_context,
			frame->width,
			frame->height,
			pixFormat,
			frame->width,
			frame->height,
			AV_PIX_FMT_RGBA,
			SWS_POINT,
			NULL,
			NULL,
			NULL); // FREE 4

		    if (coder_sws_context != NULL)
		    {
			int stride[4] = {0};
			/* stride[0]     = bitmap->w * 4; */

			av_image_fill_linesizes(stride, AV_PIX_FMT_RGBA, frame->width);

			stride[0] = FFALIGN(stride[0], 16);
			printf("WIDTH %i LINESIZE %i\n", frame->width, stride[0]);
			/* if (res < 0) */
			/* { */
			/*     fprintf(stderr, "av_image_fill_linesizes failed\n"); */
			/*     goto end; */
			/* } */
			/* for (p = 0; p < 4; p++) */
			/* { */
			/*     srcStride[p] = FFALIGN(srcStride[p], 16); */
			/*     if (srcStride[p]) */
			/* 	src[p] = av_mallocz(srcStride[p] * srcH + 16); */
			/*     if (srcStride[p] && !src[p]) */
			/*     { */
			/* 	perror("Malloc"); */
			/* 	res = -1; */
			/* 	goto end; */
			/*     } */

			bitmap    = ku_bitmap_new_aligned(frame->width, frame->height, 16); // REL 0
			stride[0] = bitmap->w * 4;

			uint8_t* scaledpixels[1];
			scaledpixels[0] = bitmap->data;

			sws_scale(
			    coder_sws_context,
			    (const uint8_t* const*) frame->data,
			    frame->linesize,
			    0,
			    bitmap->h,
			    scaledpixels,
			    stride);

			success = 1;
		    }

		    av_frame_free(&frame);   // FREE 0
		    av_packet_free(&packet); // FREE 1

		    avcodec_free_context(&codecContext);
		}
		else zc_log_error("Cannot allocate context");
	    }
	}

	avformat_close_input(&src_ctx); // CLOSE 0
    }

    avformat_free_context(src_ctx); // FREE 0

    return bitmap;
}

void coder_load_image_into(const char* path, ku_bitmap_t* bitmap)
{
    AVFormatContext* src_ctx = avformat_alloc_context(); // FREE 0

    // open the specified path
    if (avformat_open_input(&src_ctx, path, NULL, NULL) == 0) // CLOSE 0
    {
	// find the first attached picture, if available
	for (int index = 0; index < src_ctx->nb_streams; index++)
	{
	    if (src_ctx->streams[index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
	    {
		AVCodecParameters* param = src_ctx->streams[index]->codecpar;

		const AVCodec*  codec        = avcodec_find_decoder(param->codec_id);
		AVCodecContext* codecContext = avcodec_alloc_context3(codec); // FREE 1

		avcodec_parameters_to_context(codecContext, param);
		avcodec_open2(codecContext, codec, NULL); // CLOSE 1

		AVPacket* packet = av_packet_alloc(); // FREE 2
		AVFrame*  frame  = av_frame_alloc();  // FREE 3

		while (av_read_frame(src_ctx, packet) >= 0)
		{
		    avcodec_send_packet(codecContext, packet);
		    av_packet_unref(packet);
		}

		avcodec_receive_frame(codecContext, frame);

		static unsigned sws_flags = SWS_BICUBIC;

		struct SwsContext* img_convert_ctx = sws_getContext(
		    frame->width,
		    frame->height,
		    frame->format,
		    bitmap->w,
		    bitmap->h,
		    AV_PIX_FMT_ARGB,
		    sws_flags,
		    NULL,
		    NULL,
		    NULL); // FREE 4

		if (img_convert_ctx != NULL)
		{
		    uint8_t* scaledpixels[1];
		    scaledpixels[0] = bitmap->data;

		    // uint8_t* pixels[4];
		    int pitch[4];

		    pitch[0] = bitmap->w * 4;
		    sws_scale(img_convert_ctx, (const uint8_t* const*) frame->data, frame->linesize, 0, frame->height, scaledpixels, pitch);

		    sws_freeContext(img_convert_ctx); // FREE 4
		}

		av_frame_free(&frame);   // FREE 2
		av_packet_free(&packet); // FREE 3

		avcodec_close(codecContext);         // CLOSE 1
		avcodec_free_context(&codecContext); // FREE 1
	    }
	}

	avformat_close_input(&src_ctx); // CLOSE 0
    }
    else zc_log_error("cannot find file : %s", path);

    avformat_free_context(src_ctx); // FREE 0
}

int coder_load_cover_into(const char* path, ku_bitmap_t* bitmap)
{
    assert(path != NULL);

    int i   = 0;
    int ret = 0;

    AVFormatContext* src_ctx = avformat_alloc_context(); // FREE 0

    if (avformat_open_input(&src_ctx, path, NULL, NULL) != 0) zc_log_error("avformat_open_input() failed");

    // find the first attached picture, if available
    for (i = 0; i < src_ctx->nb_streams; i++)
    {
	if (src_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC)
	{
	    AVPacket           pkt   = src_ctx->streams[i]->attached_pic;
	    AVCodecParameters* param = src_ctx->streams[i]->codecpar;

	    const AVCodec*  codec        = avcodec_find_decoder(param->codec_id);
	    AVCodecContext* codecContext = avcodec_alloc_context3(codec); // FREE 1

	    avcodec_parameters_to_context(codecContext, param);
	    avcodec_open2(codecContext, codec, NULL); // CLOSE 1

	    AVFrame* frame = av_frame_alloc(); // FREE 2

	    avcodec_send_packet(codecContext, &pkt);
	    avcodec_receive_frame(codecContext, frame);

	    static unsigned sws_flags = SWS_BICUBIC;

	    struct SwsContext* img_convert_ctx = sws_getContext(
		frame->width,
		frame->height,
		frame->format,
		bitmap->w,
		bitmap->h,
		AV_PIX_FMT_ARGB,
		sws_flags,
		NULL,
		NULL,
		NULL); // FREE 3

	    if (img_convert_ctx != NULL)
	    {
		uint8_t* scaledpixels[1];
		scaledpixels[0] = malloc(bitmap->w * bitmap->h * 4);

		zc_log_debug("converting...\n");

		// uint8_t* pixels[4];
		int pitch[4];

		pitch[0] = bitmap->w * 4;
		sws_scale(img_convert_ctx, (const uint8_t* const*) frame->data, frame->linesize, 0, frame->height, scaledpixels, pitch);

		if (bitmap) ku_draw_insert_argb(bitmap, scaledpixels[0], bitmap->w, bitmap->h, 0, 0);

		sws_freeContext(img_convert_ctx); // FREE 3
		free(scaledpixels[0]);

		ret = 1;
	    }

	    avcodec_free_context(&codecContext); // FREE 1
	    avcodec_close(codecContext);         // CLOSE 1
	    av_frame_free(&frame);               // FREE 2
	}
    }

    avformat_free_context(src_ctx); // FREE 0

    return ret;
}

int coder_load_metadata_into(const char* path, map_t* map)
{
    assert(path != NULL);
    assert(map != NULL);

    int retv = -1;

    AVFormatContext* pFormatCtx  = avformat_alloc_context(); // FREE 0
    AVDictionary*    format_opts = NULL;

    av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);

    // open the specified path
    if (avformat_open_input(&pFormatCtx, path, NULL, &format_opts) == 0) // CLOSE 0
    {
	if (pFormatCtx)
	{
	    const AVOutputFormat* format = av_guess_format(NULL, path, NULL);
	    /* if (format && format->mime_type) */
	    {
		if (format && format->mime_type)
		{
		    char* slash = strstr(format->mime_type, "/");

		    if (slash)
		    {
			char* media_type = CAL(strlen(format->mime_type), NULL, cstr_describe); // REL 0
			char* container  = CAL(strlen(format->mime_type), NULL, cstr_describe); // REL 1

			memcpy(media_type, format->mime_type, slash - format->mime_type);
			memcpy(container, slash + 1, strlen(format->mime_type) - (slash - format->mime_type));

			MPUT(map, "media/type", media_type);
			MPUT(map, "media/container", container);

			REL(media_type); // REL 0
			REL(container);  // REL 1
		    }
		}

		retv = 0;

		av_dict_set(&format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);

		AVDictionaryEntry* tag = NULL;

		while ((tag = av_dict_get(pFormatCtx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
		{
		    char* value = cstr_new_cstring(tag->value);                    // REL 0
		    char* key   = cstr_new_format(100, "%s/%s", "meta", tag->key); // REL 1

		    MPUT(map, key, value);

		    REL(key);   // REL 0
		    REL(value); // REL 1
		}

		// Retrieve stream information
		retv = avformat_find_stream_info(pFormatCtx, NULL);

		if (retv >= 0)
		{
		    int dur = pFormatCtx->duration / 1000000;
		    if (dur < 0) dur = 0;
		    char* dur_s = CAL(20, NULL, cstr_describe);
		    snprintf(dur_s, 20, "%i:%.2i", (short) dur / 60, dur - (short) (dur / 60) * 60);
		    MPUT(map, "media/duration", dur_s);
		    REL(dur_s);
		}
		else
		{
		    zc_log_debug("coder_get_metadata no stream information found!!!\n");
		    MPUTR(map, "media/duration", cstr_new_cstring("0"));
		}

		MPUTR(map, "media/streams", cstr_new_format(10, "%i", pFormatCtx->nb_streams));

		for (unsigned i = 0; i < pFormatCtx->nb_streams; i++)
		{
		    AVCodecParameters* param = pFormatCtx->streams[i]->codecpar;
		    const AVCodec*     codec = avcodec_find_encoder(pFormatCtx->streams[i]->codecpar->codec_id);
		    if (codec)
		    {
			char key[100] = {0};

			if (param->codec_type == AVMEDIA_TYPE_AUDIO)
			{
			    char* channels   = CAL(10, NULL, cstr_describe); // REL 0
			    char* bitrate    = CAL(10, NULL, cstr_describe); // REL 1
			    char* samplerate = CAL(10, NULL, cstr_describe); // REL 2

			    snprintf(channels, 10, "%i", param->ch_layout.nb_channels);
			    snprintf(bitrate, 10, "%li", param->bit_rate);
			    snprintf(samplerate, 10, "%i", param->sample_rate);

			    snprintf(key, 100, "audio_%i/channels", i);
			    MPUTR(map, key, channels); // REL 0
			    snprintf(key, 100, "audio_%i/bitrate", i);
			    MPUTR(map, key, bitrate); // REL 1
			    snprintf(key, 100, "audio_%i/samplerate", i);
			    MPUTR(map, key, samplerate); // REL 2
			}
			else if (param->codec_type == AVMEDIA_TYPE_VIDEO)
			{
			    char* channels   = CAL(10, NULL, cstr_describe); // REL 0
			    char* bitrate    = CAL(10, NULL, cstr_describe); // REL 1
			    char* samplerate = CAL(10, NULL, cstr_describe); // REL 2
			    char* width      = CAL(10, NULL, cstr_describe); // REL 2
			    char* height     = CAL(10, NULL, cstr_describe); // REL 2

			    snprintf(channels, 10, "%i", param->ch_layout.nb_channels);
			    snprintf(bitrate, 10, "%li", param->bit_rate);
			    snprintf(samplerate, 10, "%i", param->sample_rate);
			    snprintf(width, 10, "%i", param->width);
			    snprintf(height, 10, "%i", param->height);

			    snprintf(key, 100, "video_%i/channels", i);
			    MPUTR(map, key, channels); // REL 0
			    snprintf(key, 100, "video_%i/bitrate", i);
			    MPUTR(map, key, bitrate); // REL 1
			    snprintf(key, 100, "video_%i/samplerate", i);
			    MPUTR(map, key, samplerate); // REL 2
			    snprintf(key, 100, "video_%i/d_width", i);
			    MPUTR(map, key, width); // REL 2
			    snprintf(key, 100, "video_%i/d_height", i);
			    MPUTR(map, key, height); // REL 2
			}
		    }
		}
	    }
	}
	else zc_log_info("coder : skpping %s, no media context present", path);

	avformat_close_input(&pFormatCtx); // CLOSE 0
    }
    else zc_log_info("coder : skipping %s, probably not a media file", path);

    if (format_opts) av_dict_free(&format_opts);

    avformat_free_context(pFormatCtx); // FREE 0

    return retv;
}

coder_media_type_t coder_get_type(const char* path)
{
    assert(path != NULL);

    coder_media_type_t type = CODER_MEDIA_TYPE_OTHER;
    int                retv = 0;

    AVFormatContext* pFormatCtx  = avformat_alloc_context(); // FREE 0
    AVDictionary*    format_opts = NULL;

    av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);

    // open the specified path
    if (avformat_open_input(&pFormatCtx, path, NULL, &format_opts) == 0) // CLOSE 0
    {
	if (pFormatCtx)
	{
	    if (pFormatCtx->duration < 0) type = CODER_MEDIA_TYPE_IMAGE;

	    // Retrieve stream information
	    retv = avformat_find_stream_info(pFormatCtx, NULL);

	    for (unsigned i = 0; i < pFormatCtx->nb_streams; i++)
	    {
		AVCodecParameters* param = pFormatCtx->streams[i]->codecpar;
		const AVCodec*     codec = avcodec_find_encoder(pFormatCtx->streams[i]->codecpar->codec_id);
		if (codec)
		{
		    if (param->codec_type == AVMEDIA_TYPE_AUDIO)
		    {
			if (type == CODER_MEDIA_TYPE_OTHER) type = CODER_MEDIA_TYPE_AUDIO;
		    }
		    else if (param->codec_type == AVMEDIA_TYPE_VIDEO)
		    {
			if (type != CODER_MEDIA_TYPE_IMAGE) type = CODER_MEDIA_TYPE_VIDEO;
		    }
		}
	    }
	}

	avformat_close_input(&pFormatCtx); // CLOSE 0
    }

    if (format_opts) av_dict_free(&format_opts);

    avformat_free_context(pFormatCtx); // FREE 0

    return type;
}

int coder_write_metadata(char* libpath, char* path, char* cover_path, map_t* changed, vec_t* drop)
{
    zc_log_info("coder_write_metadata for %s cover %s\n", path, cover_path);

    int success = 0; // indicate that everything went well after closing the avio channel

    char* ext  = path_new_extension(path); // REL 0
    char* name = path_new_filename(path);  // REL 1

    char* old_name = cstr_new_format(PATH_MAX + NAME_MAX, "%s.%s", name, ext);         // REL 2
    char* old_path = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s", libpath, path);     // REL 3
    char* new_path = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s.tmp", libpath, path); // REL 4

    REL(name); // REL 1
    REL(ext);  // REL 0

    // open file for decode

    AVFormatContext* dec_ctx = avformat_alloc_context(); // FREE 0

    if (avformat_open_input(&dec_ctx, old_path, 0, 0) >= 0)
    {
	zc_log_debug("Input opened for decoding");

	if (avformat_find_stream_info(dec_ctx, 0) >= 0)
	{
	    zc_log_debug("Input stream info found, stream count %i\n", dec_ctx->nb_streams);

	    AVFormatContext*      enc_ctx;
	    const AVOutputFormat* enc_fmt = av_guess_format(NULL, old_name, NULL);

	    if (avformat_alloc_output_context2(&enc_ctx, enc_fmt, NULL, new_path) >= 0) // FREE 1
	    {
		zc_log_debug("Output context allocated\n");

		if (!(enc_ctx->oformat->flags & AVFMT_NOFILE))
		{
		    zc_log_debug("Output file will be provided by caller.\n");

		    if (avio_open(&enc_ctx->pb, new_path, AVIO_FLAG_WRITE) >= 0) // CLOSE 0
		    {
			zc_log_debug("Output file created.\n");

			//
			// create all streams in the encoder context that are present in the decoder context
			//

			zc_log_debug("Copying stream structure...\n");

			int dec_enc_strm[10]   = {0};
			int dec_cov_strm_index = -1;

			for (int si = 0; si < dec_ctx->nb_streams; si++)
			{
			    AVCodecParameters* param = dec_ctx->streams[si]->codecpar;
			    const AVCodec*     codec = avcodec_find_encoder(dec_ctx->streams[si]->codecpar->codec_id);
			    int                cover = dec_ctx->streams[si]->disposition & AV_DISPOSITION_ATTACHED_PIC;

			    if (codec)
			    {
				zc_log_debug("Stream no %i Codec %s ID %d bit_rate %ld\n", si, codec->long_name, codec->id, param->bit_rate);

				switch (param->codec_type)
				{
				    case AVMEDIA_TYPE_VIDEO: zc_log_debug("Video Codec: resolution %d x %d\n", param->width, param->height); break;
				    case AVMEDIA_TYPE_AUDIO: zc_log_debug("Audio Codec: %d channels, sample rate %d\n", param->ch_layout.nb_channels, param->sample_rate); break;
				    default: zc_log_debug("Other codec: %i\n", param->codec_type); break;
				}

				if (cover_path && cover)
				{
				    dec_cov_strm_index = si;
				    zc_log_debug("Skipping cover stream, index : %i\n", dec_cov_strm_index);
				}
				else
				{
				    // create stream in encoder context with codec

				    AVStream* enc_stream = avformat_new_stream(enc_ctx, codec);
				    avcodec_parameters_copy(enc_stream->codecpar, param);
				    enc_stream->codecpar->codec_tag = param->codec_tag; //  do we need this?

				    // store stream index mapping

				    dec_enc_strm[si] = enc_stream->index;

				    zc_log_debug("Mapping decoder stream index %i to encoder stream index %i\n", si, enc_stream->index);
				}
			    }
			    else zc_log_error("No encoder found for stream no %i\n", si);
			}

			//
			// create cover art stream
			//

			AVFormatContext* cov_ctx       = NULL;
			int              cov_dec_index = 0;
			int              cov_enc_index = 0;

			if (cover_path)
			{
			    cov_ctx = avformat_alloc_context(); // FREE 2

			    if (avformat_open_input(&cov_ctx, cover_path, 0, 0) >= 0)
			    {
				zc_log_debug("Cover opened for decoding\n");

				if (avformat_find_stream_info(cov_ctx, 0) >= 0)
				{
				    zc_log_debug("Cover stream info found, stream count %i\n", cov_ctx->nb_streams);

				    // find stream info

				    for (int si = 0; si < cov_ctx->nb_streams; si++)
				    {
					AVCodecParameters* param = cov_ctx->streams[si]->codecpar;
					const AVCodec*     codec = avcodec_find_encoder(cov_ctx->streams[si]->codecpar->codec_id);

					if (codec)
					{
					    zc_log_debug("Cover stream no %i Codec %s ID %d bit_rate %ld\n", si, codec->long_name, codec->id, param->bit_rate);

					    switch (param->codec_type)
					    {
						case AVMEDIA_TYPE_VIDEO: zc_log_debug("Video Codec: resolution %d x %d\n", param->width, param->height); break;
						case AVMEDIA_TYPE_AUDIO: zc_log_debug("Audio Codec: %d channels, sample rate %d\n", param->ch_layout.nb_channels, param->sample_rate); break;
						default: zc_log_debug("Other codec: %i\n", param->codec_type); break;
					    }

					    // create stream in encoder context with codec

					    if (param->codec_type == AVMEDIA_TYPE_VIDEO)
					    {
						AVStream* enc_stream = avformat_new_stream(enc_ctx, codec);
						avcodec_parameters_copy(enc_stream->codecpar, param);
						enc_stream->codecpar->codec_tag = param->codec_tag; //  do we need this?
						enc_stream->disposition |= AV_DISPOSITION_ATTACHED_PIC;

						cov_dec_index = si;
						cov_enc_index = enc_stream->index;

						zc_log_debug("Mapping cover stream index %i to encoder stream index %i\n", cov_dec_index, cov_enc_index);
					    }
					}
					else zc_log_error("No encoder found for stream no %i\n", si);
				    }
				}
			    }
			}

			//
			// create metadata
			//

			av_dict_copy(&enc_ctx->metadata, dec_ctx->metadata, 0);

			AVDictionaryEntry* tag    = NULL;
			vec_t*             fields = VNEW(); // REL 0

			zc_log_debug("Existing tags:\n");
			while ((tag = av_dict_get(enc_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) zc_log_debug("%s : %s\n", tag->key, tag->value);

			map_keys(changed, fields);

			for (int fi = 0; fi < fields->length; fi++)
			{
			    char* field = fields->data[fi];
			    char* value = MGET(changed, field);
			    av_dict_set(&enc_ctx->metadata, field, value, 0);
			    zc_log_debug("added/updated %s to %s\n", field, value);
			}

			REL(fields);

			for (int fi = 0; fi < drop->length; fi++)
			{
			    char* field = drop->data[fi];
			    av_dict_set(&enc_ctx->metadata, field, NULL, 0);
			    zc_log_debug("removed %s\n", field);
			}

			if (avformat_init_output(enc_ctx, NULL) > 0)
			{
			    zc_log_debug("Output inited.\n");

			    // write header

			    if (avformat_write_header(enc_ctx, NULL) >= 0)
			    {
				zc_log_debug("Header written\n");

				// copy packets from decoder context to encoder context

				AVPacket* dec_pkt = av_packet_alloc(); // FREE 0

				// av_init_packet(dec_pkt);

				dec_pkt->data = NULL;
				dec_pkt->size = 0;

				// copy all packets from old file to new file with the exception of cover art image

				int last_di = -1; // last decoder index
				int last_ei = 0;  // last encoder index
				int last_pc = 0;  // last packet count
				int last_by = 0;  // last sum bytes

				while (av_read_frame(dec_ctx, dec_pkt) == 0)
				{
				    if (cover_path && dec_pkt->stream_index == dec_cov_strm_index)
				    {
					zc_log_debug("Skipping original cover packets.\n");
				    }
				    else
				    {

					if (last_di == -1) last_di = dec_pkt->stream_index;

					int enc_index = dec_enc_strm[dec_pkt->stream_index];

					if (dec_pkt->stream_index == last_di)
					{
					    last_by += dec_pkt->size;
					    last_pc += 1;
					    last_ei = enc_index;
					}
					else
					{
					    zc_log_debug("Stream written, dec index : %i enc index : %i packets : %i sum : %i bytes\n", last_di, last_ei, last_pc, last_by);
					    last_di = dec_pkt->stream_index;
					    last_ei = enc_index;
					    last_pc = 0;
					    last_by = 0;
					}

					dec_pkt->stream_index = enc_index;
					av_write_frame(enc_ctx, dec_pkt);
					dec_pkt->data = NULL;
					dec_pkt->size = 0;
				    }
				}

				zc_log_debug("Stream written, dec index : %i enc index : %i packets : %i sum : %i bytes\n", last_di, last_ei, last_pc, last_by);

				last_pc = 0;
				last_by = 0;

				if (cover_path)
				{
				    // write cover packets

				    while (av_read_frame(cov_ctx, dec_pkt) == 0)
				    {
					if (dec_pkt->stream_index == cov_dec_index)
					{
					    last_by += dec_pkt->size;
					    last_pc += 1;
					    dec_pkt->stream_index = cov_enc_index;
					    av_write_frame(enc_ctx, dec_pkt);
					}

					dec_pkt->data = NULL;
					dec_pkt->size = 0;
				    }
				    zc_log_debug("Cover stream written, dec index : %i enc index : %i packets : %i sum : %i bytes\n", cov_dec_index, cov_enc_index, last_pc, last_by);
				}

				// close output file and cleanup

				av_packet_free(&dec_pkt);
				av_write_trailer(enc_ctx);

				success = 1;
			    }
			    else zc_log_error("Cannot write header.\n");
			}
			else zc_log_error("Cannot init output.\n");

			if (cov_ctx) avformat_free_context(cov_ctx);

			if (avio_close(enc_ctx->pb) >= 0) zc_log_debug("Closed target file.\n"); // CLOSE 0

			if (success)
			{
			    if (rename(new_path, old_path) == 0) zc_log_debug("Temporary file renamed to original file.\n");
			    else zc_log_error("Couldn't rename temporary file to original file.\n");
			}
		    }
		    else zc_log_error("Cannot open file for encode %s\n", new_path);
		}
		else zc_log_error("Output is a fileless codec.");

		avformat_free_context(enc_ctx); // FREE 1
	    }
	    else zc_log_error("Cannot allocate output context for %s\n", old_name);
	}
	else zc_log_error("Cannot find stream info\n");

	avformat_close_input(&dec_ctx);
    }
    else zc_log_error("Cannot open file for decode %s\n", old_path);

    avformat_free_context(dec_ctx); // FREE 0

    REL(old_name); // REL 2
    REL(old_path); // REL 3
    REL(new_path); // REL 4

    if (success)
	return 0;
    else
	return -1;
}

int coder_write_png(char* path, ku_bitmap_t* bm)
{
    int            success = 0;
    const AVCodec* codec   = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (codec)
    {
	AVCodecContext* enc_ctx = avcodec_alloc_context3(codec); // FREE 0
	if (enc_ctx)
	{
	    enc_ctx->width     = bm->w;
	    enc_ctx->height    = bm->h;
	    enc_ctx->time_base = (AVRational){1, 25};
	    enc_ctx->pix_fmt   = AV_PIX_FMT_RGBA;

	    if (avcodec_open2(enc_ctx, codec, NULL) >= 0) // CLOSE 0
	    {
		AVFrame* frame_in = av_frame_alloc(); // FREE 1
		frame_in->format  = AV_PIX_FMT_RGBA;
		frame_in->width   = bm->w;
		frame_in->height  = bm->h;

		av_image_fill_arrays(frame_in->data, frame_in->linesize, bm->data, AV_PIX_FMT_RGBA, bm->w, bm->h, 1);

		AVPacket* pkt = av_packet_alloc();

		if (avcodec_send_frame(enc_ctx, frame_in) >= 0)
		{
		    if (avcodec_receive_packet(enc_ctx, pkt) >= 0)
		    {
			FILE* file = fopen(path, "wb");
			fwrite(pkt->data, 1, pkt->size, file);
			fclose(file);
			av_packet_free(&pkt);
		    }
		    else zc_log_error("Error during encoding\n");
		}
		else zc_log_error("Error during encoding\n");

		av_frame_free(&frame_in); // FREE 1
		avcodec_close(enc_ctx);   // CLOSE 0
	    }
	    else zc_log_error("Could not open codec\n");

	    av_free(enc_ctx); // FREE 0
	}
	else zc_log_error("Could not allocate video codec context\n");
    }
    else zc_log_error("Codec not found\n");

    if (success)
	return 0;
    else
	return -1;
}

#endif

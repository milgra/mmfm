#ifndef coder_h
#define coder_h

#include "zc_bitmap.c"
#include "zc_map.c"
#include "zc_vector.c"

bm_t* coder_load_image(const char* path);
void  coder_load_image_into(const char* path, bm_t* bitmap);
void  coder_load_cover_into(const char* path, bm_t* bitmap);
int   coder_load_metadata_into(const char* path, map_t* map);
int   coder_write_metadata(char* libpath, char* path, char* cover_path, map_t* data, vec_t* drop);
int   coder_write_png(char* path, bm_t* bm);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "zc_cstring.c"
#include "zc_cstrpath.c"
#include "zc_graphics.c"
#include "zc_log.c"
#include "zc_memory.c"
#include <limits.h>

bm_t* coder_load_image(const char* path)
{
  bm_t* bitmap  = NULL;
  int   success = 0;

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

        avcodec_parameters_to_context(codecContext, param);
        avcodec_open2(codecContext, codec, NULL); // CLOSE 1

        AVPacket* packet = av_packet_alloc(); // FREE 2
        AVFrame*  frame  = av_frame_alloc();  // FREE 3

        while (av_read_frame(src_ctx, packet) >= 0)
        {
          avcodec_send_packet(codecContext, packet);
        }

        avcodec_receive_frame(codecContext, frame);

        static unsigned sws_flags = SWS_BICUBIC;

        struct SwsContext* img_convert_ctx = sws_getContext(frame->width,
                                                            frame->height,
                                                            frame->format,
                                                            frame->width,
                                                            frame->height,
                                                            AV_PIX_FMT_RGBA,
                                                            sws_flags,
                                                            NULL,
                                                            NULL,
                                                            NULL); // FREE 4

        if (img_convert_ctx != NULL)
        {
          bitmap = bm_new(frame->width, frame->height); // REL 0

          uint8_t* scaledpixels[1];
          scaledpixels[0] = malloc(bitmap->w * bitmap->h * 4);

          uint8_t* pixels[4];
          int      pitch[4];

          pitch[0] = bitmap->w * 4;
          sws_scale(img_convert_ctx,
                    (const uint8_t* const*)frame->data,
                    frame->linesize,
                    0,
                    frame->height,
                    scaledpixels,
                    pitch);

          gfx_rect(bitmap, 0, 0, bitmap->w, bitmap->h, 0xFF0000FF, 0);

          gfx_insert_rgba(bitmap,
                          scaledpixels[0],
                          bitmap->w,
                          bitmap->h,
                          0,
                          0);

          sws_freeContext(img_convert_ctx); // FREE 4

          success = 1;
        }

        av_frame_free(&frame);   // FREE 0
        av_packet_free(&packet); // FREE 1

        avcodec_free_context(&codecContext);
        avcodec_close(codecContext);
      }
    }

    avformat_close_input(&src_ctx); // CLOSE 0
  }

  avformat_free_context(src_ctx); // FREE 0

  return bitmap;
}

void coder_load_image_into(const char* path, bm_t* bitmap)
{
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

        avcodec_parameters_to_context(codecContext, param);
        avcodec_open2(codecContext, codec, NULL); // CLOSE 1

        AVPacket* packet = av_packet_alloc(); // FREE 2
        AVFrame*  frame  = av_frame_alloc();  // FREE 3

        while (av_read_frame(src_ctx, packet) >= 0)
        {
          avcodec_send_packet(codecContext, packet);
        }

        avcodec_receive_frame(codecContext, frame);

        static unsigned sws_flags = SWS_BICUBIC;

        struct SwsContext* img_convert_ctx = sws_getContext(frame->width,
                                                            frame->height,
                                                            frame->format,
                                                            bitmap->w,
                                                            bitmap->h,
                                                            AV_PIX_FMT_RGBA,
                                                            sws_flags,
                                                            NULL,
                                                            NULL,
                                                            NULL); // FREE 4

        if (img_convert_ctx != NULL)
        {
          uint8_t* scaledpixels[1];
          scaledpixels[0] = malloc(bitmap->w * bitmap->h * 4);

          uint8_t* pixels[4];
          int      pitch[4];

          pitch[0] = bitmap->w * 4;
          sws_scale(img_convert_ctx,
                    (const uint8_t* const*)frame->data,
                    frame->linesize,
                    0,
                    frame->height,
                    scaledpixels,
                    pitch);

          gfx_rect(bitmap, 0, 0, bitmap->w, bitmap->h, 0xFF0000FF, 0);

          gfx_insert_rgba(bitmap,
                          scaledpixels[0],
                          bitmap->w,
                          bitmap->h,
                          0,
                          0);

          free(scaledpixels[0]);
          sws_freeContext(img_convert_ctx); // FREE 4
        }

        av_frame_free(&frame); // FREE 2
        av_packet_unref(packet);
        av_packet_free(&packet); // FREE 3

        avcodec_free_context(&codecContext); // FREE 1
        avcodec_close(codecContext);         // CLOSE 1
      }
    }

    avformat_close_input(&src_ctx); // CLOSE 0
  }

  avformat_free_context(src_ctx); // FREE 0
}

void coder_load_cover_into(const char* path, bm_t* bitmap)
{
  assert(path != NULL);

  printf("coder_get_album %s %i %i\n", path, bitmap->w, bitmap->h);

  int i, ret = 0;

  AVFormatContext* src_ctx = avformat_alloc_context(); // FREE 0

  /* // open the specified path */
  if (avformat_open_input(&src_ctx, path, NULL, NULL) != 0)
  {
    printf("avformat_open_input() failed");
  }

  bm_t* result = NULL;

  // find the first attached picture, if available
  for (i = 0; i < src_ctx->nb_streams; i++)
  {
    if (src_ctx->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC)
    {
      AVPacket pkt = src_ctx->streams[i]->attached_pic;

      printf("ALBUM ART SIZE %i\n", pkt.size);

      AVCodecParameters* param = src_ctx->streams[i]->codecpar;

      printf("Resolution %d x %d codec %i\n", param->width, param->height, param->codec_id);

      const AVCodec*  codec        = avcodec_find_decoder(param->codec_id);
      AVCodecContext* codecContext = avcodec_alloc_context3(codec); // FREE 1

      avcodec_parameters_to_context(codecContext, param);
      avcodec_open2(codecContext, codec, NULL); // CLOSE 1

      AVFrame* frame = av_frame_alloc(); // FREE 2

      avcodec_send_packet(codecContext, &pkt);
      avcodec_receive_frame(codecContext, frame);

      printf("received frame %i %i\n", frame->width, frame->height);

      static unsigned sws_flags = SWS_BICUBIC;

      struct SwsContext* img_convert_ctx = sws_getContext(frame->width,
                                                          frame->height,
                                                          frame->format,
                                                          bitmap->w,
                                                          bitmap->h,
                                                          AV_PIX_FMT_RGBA,
                                                          sws_flags,
                                                          NULL,
                                                          NULL,
                                                          NULL); // FREE 3

      if (img_convert_ctx != NULL)
      {
        uint8_t* scaledpixels[1];
        scaledpixels[0] = malloc(bitmap->w * bitmap->h * 4);

        printf("converting...\n");

        uint8_t* pixels[4];
        int      pitch[4];

        pitch[0] = bitmap->w * 4;
        sws_scale(img_convert_ctx,
                  (const uint8_t* const*)frame->data,
                  frame->linesize,
                  0,
                  frame->height,
                  scaledpixels,
                  pitch);

        if (bitmap)
        {
          gfx_insert_rgb(bitmap,
                         scaledpixels[0],
                         bitmap->w,
                         bitmap->h,
                         0,
                         0);
        }

        sws_freeContext(img_convert_ctx); // FREE 3
      }

      avcodec_free_context(&codecContext); // FREE 1
      avcodec_close(codecContext);         // CLOSE 1
      av_frame_free(&frame);               // FREE 2
    }
  }

  avformat_free_context(src_ctx); // FREE 0
}

int coder_load_metadata_into(const char* path, map_t* map)
{
  assert(path != NULL);
  assert(map != NULL);

  int retv = -1;

  AVFormatContext* pFormatCtx  = avformat_alloc_context(); // FREE 0
  AVDictionary*    format_opts = NULL;

  av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);

  /* // open the specified path */
  if (avformat_open_input(&pFormatCtx, path, NULL, &format_opts) == 0) // CLOSE 0
  {
    if (pFormatCtx)
    {
      AVOutputFormat* format = av_guess_format(NULL, path, NULL);
      if (format && format->mime_type)
      {
        char* slash = strstr(format->mime_type, "/");

        if (slash)
        {
          char* media_type = CAL(strlen(format->mime_type), NULL, cstr_describe); // REL 0
          char* container  = CAL(strlen(format->mime_type), NULL, cstr_describe); // REL 1

          memcpy(media_type, format->mime_type, slash - format->mime_type);
          memcpy(container, slash + 1, strlen(format->mime_type) - (slash - format->mime_type));

          MPUT(map, "file/media_type", media_type);
          MPUT(map, "file/container", container);

          REL(media_type); // REL 0
          REL(container);  // REL 1
        }
      }
      else
      {
        printf("cannot guess format for %s\n", path);
        return retv;
      }

      if (strcmp(MGET(map, "file/media_type"), "audio") != 0 && strcmp(MGET(map, "file/media_type"), "video") != 0)
      {
        printf("not audio not video\n");
        return retv;
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
        int   dur   = pFormatCtx->duration / 1000000;
        char* dur_s = CAL(10, NULL, cstr_describe);
        snprintf(dur_s, 10, "%i:%.2i", (int)dur / 60, dur - (int)(dur / 60) * 60);
        MPUT(map, "file/duration", dur_s);
        REL(dur_s);
      }
      else
      {
        printf("coder_get_metadata no stream information found!!!\n");
        MPUTR(map, "file/duration", cstr_new_cstring("0"));
      }

      for (unsigned i = 0; i < pFormatCtx->nb_streams; i++)
      {
        AVCodecParameters* param = pFormatCtx->streams[i]->codecpar;
        const AVCodec*     codec = avcodec_find_encoder(pFormatCtx->streams[i]->codecpar->codec_id);
        if (codec)
        {
          if (param->codec_type == AVMEDIA_TYPE_AUDIO)
          {
            char* channels   = CAL(10, NULL, cstr_describe); // REL 0
            char* bitrate    = CAL(10, NULL, cstr_describe); // REL 1
            char* samplerate = CAL(10, NULL, cstr_describe); // REL 2

            snprintf(channels, 10, "%i", param->channels);
            snprintf(bitrate, 10, "%li", param->bit_rate);
            snprintf(samplerate, 10, "%i", param->sample_rate);

            MPUTR(map, "file/channels", channels);      // REL 0
            MPUTR(map, "file/bit_rate", bitrate);       // REL 1
            MPUTR(map, "file/sample_rate", samplerate); // REL 2
          }
        }
      }
    }
    else
    {
      LOG("coder : skpping %s, no media context present", path);
    }

    avformat_close_input(&pFormatCtx); // CLOSE 0
  }
  else
  {
    LOG("coder : skipping %s, probably not a media file", path);
  }

  avformat_free_context(pFormatCtx); // FREE 0

  return retv;
}

int coder_write_metadata(char* libpath, char* path, char* cover_path, map_t* data, vec_t* drop)
{
  LOG("coder_write_metadata for %s cover %s\n", path, cover_path);

  int success = 0; // indicate that everything went well after closing the avio channel

  char* ext  = cstr_new_path_extension(path); // REL 0
  char* name = cstr_new_path_filename(path);  // REL 1

  char* old_name = cstr_new_format(PATH_MAX + NAME_MAX, "%s.%s", name, ext);         // REL 2
  char* old_path = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s", libpath, path);     // REL 3
  char* new_path = cstr_new_format(PATH_MAX + NAME_MAX, "%s/%s.tmp", libpath, path); // REL 4

  printf("old_path %s\n", old_path);
  printf("new_path %s\n", new_path);

  REL(name); // REL 1
  REL(ext);  // REL 0

  // open file for decode

  AVFormatContext* dec_ctx = avformat_alloc_context(); // FREE 0

  if (avformat_open_input(&dec_ctx, old_path, 0, 0) >= 0)
  {
    printf("Input opened for decoding\n");

    if (avformat_find_stream_info(dec_ctx, 0) >= 0)
    {
      printf("Input stream info found, stream count %i\n", dec_ctx->nb_streams);

      AVFormatContext* enc_ctx;
      AVOutputFormat*  enc_fmt = av_guess_format(NULL, old_name, NULL);

      if (avformat_alloc_output_context2(&enc_ctx, enc_fmt, NULL, new_path) >= 0) // FREE 1
      {
        printf("Output context allocated\n");

        if (!(enc_ctx->oformat->flags & AVFMT_NOFILE))
        {
          printf("Output file will be provided by caller.\n");

          if (avio_open(&enc_ctx->pb, new_path, AVIO_FLAG_WRITE) >= 0) // CLOSE 0
          {
            printf("Output file created.\n");

            //
            // create all streams in the encoder context that are present in the decoder context
            //

            printf("Copying stream structure...\n");

            int dec_enc_strm[10]   = {0};
            int dec_cov_strm_index = -1;

            for (int si = 0; si < dec_ctx->nb_streams; si++)
            {
              AVCodecParameters* param = dec_ctx->streams[si]->codecpar;
              const AVCodec*     codec = avcodec_find_encoder(dec_ctx->streams[si]->codecpar->codec_id);
              int                cover = dec_ctx->streams[si]->disposition & AV_DISPOSITION_ATTACHED_PIC;

              if (codec)
              {
                printf("Stream no %i Codec %s ID %d bit_rate %ld\n", si, codec->long_name, codec->id, param->bit_rate);

                switch (param->codec_type)
                {
                case AVMEDIA_TYPE_VIDEO: printf("Video Codec: resolution %d x %d\n", param->width, param->height); break;
                case AVMEDIA_TYPE_AUDIO: printf("Audio Codec: %d channels, sample rate %d\n", param->channels, param->sample_rate); break;
                default: printf("Other codec: %i\n", param->codec_type); break;
                }

                if (cover_path && cover)
                {
                  dec_cov_strm_index = si;
                  printf("Skipping cover stream, index : %i\n", dec_cov_strm_index);
                }
                else
                {
                  // create stream in encoder context with codec

                  AVStream* enc_stream = avformat_new_stream(enc_ctx, codec);
                  avcodec_parameters_copy(enc_stream->codecpar, param);
                  enc_stream->codecpar->codec_tag = param->codec_tag; //  do we need this?

                  // store stream index mapping

                  dec_enc_strm[si] = enc_stream->index;

                  printf("Mapping decoder stream index %i to encoder stream index %i\n", si, enc_stream->index);
                }
              }
              else
                printf("No encoder found for stream no %i\n", si);
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
                printf("Cover opened for decoding\n");

                if (avformat_find_stream_info(cov_ctx, 0) >= 0)
                {
                  printf("Cover stream info found, stream count %i\n", cov_ctx->nb_streams);

                  // find stream info

                  for (int si = 0; si < cov_ctx->nb_streams; si++)
                  {
                    AVCodecParameters* param = cov_ctx->streams[si]->codecpar;
                    const AVCodec*     codec = avcodec_find_encoder(cov_ctx->streams[si]->codecpar->codec_id);

                    if (codec)
                    {
                      printf("Cover stream no %i Codec %s ID %d bit_rate %ld\n", si, codec->long_name, codec->id, param->bit_rate);

                      switch (param->codec_type)
                      {
                      case AVMEDIA_TYPE_VIDEO: printf("Video Codec: resolution %d x %d\n", param->width, param->height); break;
                      case AVMEDIA_TYPE_AUDIO: printf("Audio Codec: %d channels, sample rate %d\n", param->channels, param->sample_rate); break;
                      default: printf("Other codec: %i\n", param->codec_type); break;
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

                        printf("Mapping cover stream index %i to encoder stream index %i\n", cov_dec_index, cov_enc_index);
                      }
                    }
                    else
                      printf("No encoder found for stream no %i\n", si);
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

            printf("Existing tags:\n");
            while ((tag = av_dict_get(enc_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) printf("%s : %s\n", tag->key, tag->value);

            map_keys(data, fields);

            for (int fi = 0; fi < fields->length; fi++)
            {
              char* field = fields->data[fi];
              char* value = MGET(data, field);
              av_dict_set(&enc_ctx->metadata, field + 5, value, 0);
              printf("added/updated %s to %s\n", field + 5, value);
            }

            REL(fields);

            for (int fi = 0; fi < drop->length; fi++)
            {
              char* field = drop->data[fi];
              av_dict_set(&enc_ctx->metadata, field + 5, NULL, 0);
              printf("removed %s\n", field + 5);
            }

            if (avformat_init_output(enc_ctx, NULL) > 0)
            {
              printf("Output inited.\n");

              // write header

              if (avformat_write_header(enc_ctx, NULL) >= 0)
              {
                printf("Header written\n");

                // copy packets from decoder context to encoder context

                AVPacket* dec_pkt = av_packet_alloc(); // FREE 0

                av_init_packet(dec_pkt);

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
                    printf("Skipping original cover packets.\n");
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
                      printf("Stream written, dec index : %i enc index : %i packets : %i sum : %i bytes\n", last_di, last_ei, last_pc, last_by);
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

                printf("Stream written, dec index : %i enc index : %i packets : %i sum : %i bytes\n", last_di, last_ei, last_pc, last_by);

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
                  printf("Cover stream written, dec index : %i enc index : %i packets : %i sum : %i bytes\n", cov_dec_index, cov_enc_index, last_pc, last_by);
                }

                // close output file and cleanup

                av_packet_free(&dec_pkt);
                av_write_trailer(enc_ctx);

                success = 1;
              }
              else
                printf("Cannot write header.\n");
            }
            else
              printf("Cannot init output.\n");

            if (cov_ctx) avformat_free_context(cov_ctx);

            if (avio_close(enc_ctx->pb) >= 0) printf("Closed target file.\n"); // CLOSE 0

            if (success)
            {
              if (rename(new_path, old_path) == 0)
                printf("Temporary file renamed to original file.\n");
              else
                printf("Couldn't rename temporary file to original file.\n");
            }
          }
          else
            printf("Cannot open file for encode %s\n", new_path);
        }
        else
          printf("Output is a fileless codec.");

        avformat_free_context(enc_ctx); // FREE 1
      }
      else
        printf("Cannot allocate output context for %s\n", old_name);
    }
    else
      printf("Cannot find stream info\n");

    avformat_close_input(&dec_ctx);
  }
  else
    printf("Cannot open file for decode %s\n", old_path);

  avformat_free_context(dec_ctx); // FREE 0

  REL(old_name); // REL 2
  REL(old_path); // REL 3
  REL(new_path); // REL 4

  if (success)
    return 0;
  else
    return -1;
}

int coder_write_png(char* path, bm_t* bm)
{
  int      success;
  AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
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

        av_image_fill_arrays(frame_in->data,
                             frame_in->linesize,
                             bm->data,
                             AV_PIX_FMT_RGBA,
                             bm->w,
                             bm->h,
                             1);

        AVPacket pkt;
        pkt.data = NULL;
        pkt.size = 0;
        av_init_packet(&pkt);

        if (avcodec_send_frame(enc_ctx, frame_in) >= 0)
        {
          if (avcodec_receive_packet(enc_ctx, &pkt) >= 0)
          {
            FILE* file = fopen(path, "wb");
            fwrite(pkt.data, 1, pkt.size, file);
            fclose(file);
            av_packet_unref(&pkt);
          }
          else
            printf("Error during encoding\n");
        }
        else
          printf("Error during encoding\n");

        av_frame_free(&frame_in); // FREE 1
        avcodec_close(enc_ctx);   // CLOSE 0
      }
      else
        printf("Could not open codec\n");

      av_free(enc_ctx); // FREE 0
    }
    else
      printf("Could not allocate video codec context\n");
  }
  else
    printf("Codec not found\n");

  if (success)
    return 0;
  else
    return -1;
}

#endif

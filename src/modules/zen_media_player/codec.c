#ifndef codec_h
#define codec_h

#include "libavformat/avformat.h"

AVDictionary* filter_codec_opts(AVDictionary* opts, enum AVCodecID codec_id, AVFormatContext* s, AVStream* st, AVCodec* codec);

#endif

#if __INCLUDE_LEVEL__ == 0

#include "libavutil/opt.h"

int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec)
{
  int ret = avformat_match_stream_specifier(s, st, spec);
  if (ret < 0)
    av_log(s, AV_LOG_ERROR, "Invalid stream specifier: %s.\n", spec);
  return ret;
}

AVDictionary* filter_codec_opts(AVDictionary* opts, enum AVCodecID codec_id, AVFormatContext* s, AVStream* st, AVCodec* codec)
{
  AVDictionary*      ret    = NULL;
  AVDictionaryEntry* t      = NULL;
  int                flags  = s->oformat ? AV_OPT_FLAG_ENCODING_PARAM : AV_OPT_FLAG_DECODING_PARAM;
  char               prefix = 0;
  const AVClass*     cc     = avcodec_get_class();

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
  default:
    break;
  }

  while ((t = av_dict_get(opts, "", t, AV_DICT_IGNORE_SUFFIX)))
  {
    char* p = strchr(t->key, ':');

    /* check stream specification in opt name */
    if (p)
      switch (check_stream_specifier(s, st, p + 1))
      {
      case 1:
        *p = 0;
        break;
      case 0:
        continue;
      default:
        exit(1);
      }

    if (av_opt_find(&cc, t->key, NULL, flags, AV_OPT_SEARCH_FAKE_OBJ) ||
        !codec ||
        (codec->priv_class &&
         av_opt_find(&codec->priv_class, t->key, NULL, flags,
                     AV_OPT_SEARCH_FAKE_OBJ)))
      av_dict_set(&ret, t->key, t->value, 0);
    else if (t->key[0] == prefix &&
             av_opt_find(&cc, t->key + 1, NULL, flags,
                         AV_OPT_SEARCH_FAKE_OBJ))
      av_dict_set(&ret, t->key + 1, t->value, 0);

    if (p)
      *p = ':';
  }
  return ret;
}

#endif

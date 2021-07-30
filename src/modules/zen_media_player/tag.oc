int update_tags(int argc, char** argv)
{

  av_register_all();
  avcodec_register_all();

  AVFormatContext* ctx;
  std::string      path("./input.mp3");

  ctx = avformat_alloc_context();

  if (avformat_open_input(&ctx, path.c_str(), 0, 0) < 0)
    std::cout << "error1" << std::endl;

  if (avformat_find_stream_info(ctx, 0) < 0)
    std::cout << "error2" << std::endl;

  AVDictionaryEntry* tag = nullptr;
  tag                    = av_dict_get(ctx->metadata, "artist", tag, AV_DICT_IGNORE_SUFFIX);
  std::cout << tag->key << " : " << tag->value << std::endl;

  av_dict_set(&ctx->metadata, "TPFL", "testtest", 0);

  std::cout << "test!" << std::endl;

  int status;

  AVFormatContext* ofmt_ctx;
  AVOutputFormat*  ofmt = av_guess_format("mp3", "./out.mp3", NULL);
  status                = avformat_alloc_output_context2(&ofmt_ctx, ofmt, "mp3", "./out.mp3");
  if (status < 0)
  {
    std::cerr << "could not allocate output format" << std::endl;
    return 0;
  }

  int audio_stream_index = 0;
  for (unsigned i = 0; i < ctx->nb_streams; i++)
  {
    if (ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audio_stream_index = i;
      const AVCodec* c   = avcodec_find_encoder(ctx->streams[i]->codecpar->codec_id);
      if (c)
      {
        AVStream* ostream = avformat_new_stream(ofmt_ctx, c);
        avcodec_parameters_copy(ostream->codecpar, ctx->streams[i]->codecpar);
        ostream->codecpar->codec_tag = 0;
      }
      break;
    }
  }

  av_dict_set(&ofmt_ctx->metadata, "TPFL", "testtest", 0);

  av_dump_format(ofmt_ctx, 0, "./out.mp3", 1);

  if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
  {
    avio_open(&ofmt_ctx->pb, "./out.mp3", AVIO_FLAG_WRITE);
  }

  if (avformat_init_output(ofmt_ctx, NULL) == AVSTREAM_INIT_IN_WRITE_HEADER)
  {
    status = avformat_write_header(ofmt_ctx, NULL);
  }

  AVPacket* pkt = av_packet_alloc();
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;

  while (av_read_frame(ctx, pkt) == 0)
  {
    if (pkt->stream_index == audio_stream_index)
    {
      // this is optional, we are copying the stream
      av_packet_rescale_ts(pkt, ctx->streams[audio_stream_index]->time_base,
                           ofmt_ctx->streams[audio_stream_index]->time_base);
      av_write_frame(ofmt_ctx, pkt);
    }
  }

  av_packet_free(&pkt);

  av_write_trailer(ofmt_ctx);

  avformat_close_input(&ctx);
  avformat_free_context(ofmt_ctx);
  avformat_free_context(ctx);

  if (status == 0)
    std::cout << "test1" << std::endl;

  return 0;
}

// filter code sample

void VideoManager::init_filter_graph(AVFrame* frame) {

  if (filter_initialised)
    return;

  int result;

  AVFilter* buffer_src   = avfilter_get_by_name ("buffer");
  AVFilter* buffer_sink  = avfilter_get_by_name ("buffersink");
  AVFilterInOut* inputs  = avfilter_inout_alloc();
  AVFilterInOut* outputs = avfilter_inout_alloc();

  AVCodecContext* ctx = ffmpeg.vid_stream.context;
  char args[512];

  int frame_fix = 0; // fix bad width on some streams
  if (frame->width < 704)
    frame_fix = 2;
  else if (frame->width > 704)
    frame_fix = -16;

  snprintf (args, sizeof(args),
            "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
            frame->width + frame_fix,
            frame->height,
            frame->format,// ctx->pix_fmt,
            ctx->time_base.num,
            ctx->time_base.den,
            ctx->sample_aspect_ratio.num,
            ctx->sample_aspect_ratio.den);

  const char* description = "yadif=1:-1:0";
  LOGD ("Filter: %s - Settings: %s", description, args);
  filter_graph = avfilter_graph_alloc();
  result = avfilter_graph_create_filter (&filter_src_ctx, buffer_src, "in", args, NULL, filter_graph);
  if (result < 0) {
    LOGI ("Filter graph - Unable to create buffer source");
    return;
    }

  AVBufferSinkParams *params = av_buffersink_params_alloc();
  enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };

  params->pixel_fmts = pix_fmts;
  result = avfilter_graph_create_filter (&filter_sink_ctx, buffer_sink, "out", NULL, params, filter_graph);
  av_free(params);
  if (result < 0) {
    LOGI ("Filter graph - Unable to create buffer sink");
    return;
    }

  inputs->name        = av_strdup ("out");
  inputs->filter_ctx  = filter_sink_ctx;
  inputs->pad_idx     = 0;
  inputs->next        = NULL;

  outputs->name       = av_strdup ("in");
  outputs->filter_ctx = filter_src_ctx;
  outputs->pad_idx    = 0;
  outputs->next       = NULL;

  result = avfilter_graph_parse_ptr (filter_graph, description, &inputs, &outputs, NULL);
  if (result < 0)
    LOGI ("avfilter_graph_parse_ptr ERROR");

  result = avfilter_graph_config (filter_graph, NULL);
  if (result < 0)
    LOGI ("avfilter_graph_config ERROR");

  filter_initialised =
  }

void FFMPEG::process_video_packet (AVPacket *pkt) {

  int got;
  AVFrame* frame = vid_stream.frame;
  avcodec_decode_video2 (vid_stream.context, frame, &got, pkt);

  if (got) {
    if (!frame->interlaced_frame)
      // not interlaced
      Video.add_av_frame (frame, 0);
    else {
      if (!Video.filter_initialised)
        Video.init_filter_graph (frame);

      av_buffersrc_add_frame_flags (Video.filter_src_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF);
      int c = 0;

      while (true) {
        AVFrame *filter_frame = ffmpeg.vid_stream.filter_frame;

        int result = av_buffersink_get_frame (Video.filter_sink_ctx, filter_frame);

        if (result == AVERROR(EAGAIN) || result == AVERROR(AVERROR_EOF))
          break;
        if (result < 0)
          return;

        Video.add_av_frame (filter_frame, c++);
        av_frame_unref (filter_frame);
        }
      }
    }
  }

// cFFmpegVideoDecoder.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <algorithm>
#include <functional>

#include "../common/cLog.h"

#include "cDecoder.h"
#include "cFrame.h"
//}}}

namespace {
  //{{{
  void logCallback (void* ptr, int level, const char* fmt, va_list vargs) {
    (void)level;
    (void)ptr;
    (void)fmt;
    (void)vargs;
    //vprintf (fmt, vargs);
    }
  //}}}
  }

class cFFmpegVideoDecoder : public cDecoder {
public:
  //{{{
  cFFmpegVideoDecoder (cRender& render, uint8_t streamType)
      : cDecoder(),
        mRender(render),
        mStreamType(streamType), mH264(mStreamType == 27), mStreamName(mH264 ? "h264" : "mpeg2"),
        mAvCodec(avcodec_find_decoder (mH264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO)) {

    av_log_set_level (AV_LOG_ERROR);
    av_log_set_callback (logCallback);

    cLog::log (LOGINFO, fmt::format ("cFFmpegVideoDecoder - streamType:{}:{}", mStreamType, mStreamName));

    mAvParser = av_parser_init (mH264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    mAvContext->flags2 |= AV_CODEC_FLAG2_FAST;

    avcodec_open2 (mAvContext, mAvCodec, nullptr);

    //AVDictionary* opts = nullptr;
    //av_dict_set (&opts, "flags2", "+export_mvs", 0);
    //avcodec_open2 (mAvContext, mAvCodec, &opts);
    //av_dict_free (&opts);
    }
  //}}}
  //{{{
  virtual ~cFFmpegVideoDecoder() {

    if (mAvContext)
      avcodec_close (mAvContext);

    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  virtual std::string getInfoString() const final { return mH264 ? "ffmpeg h264" : "ffmpeg mpeg"; }
  //{{{
  virtual int64_t decode (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          std::function<cFrame*()> allocFrameCallback,
                          std::function<void (cFrame* frame)> addFrameCallback) final {

    AVFrame* avFrame = av_frame_alloc();
    AVPacket* avPacket = av_packet_alloc();
    uint8_t* frame = pes;
    uint32_t frameSize = pesSize;
    while (frameSize) {
      auto now = std::chrono::system_clock::now();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                        frame, (int)frameSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;

          //{{{  sideData
          //AVFrameSideData* sd = av_frame_get_side_data (avFrame, AV_FRAME_DATA_MOTION_VECTORS);
          //if (sd) {
          //  const AVMotionVector* mvs = (const AVMotionVector*)sd->data;
          //  for (int i = 0; i < sd->size / sizeof(*mvs); i++) {
          //    cLog::log (LOGINFO, fmt::format ("mvs {}", sd->size / sizeof(*mvs)));
              //const AVMotionVector* mv = &mvs[i];
              //cLog::log (LOGINFO, fmt::format ("{} {} {}x{} {}x{} {}x{} {} {}x{} {}",
              //                                 i, mv->source,
              //                                 mv->w, mv->h, mv->src_x, mv->src_y,
              //                                 mv->dst_x, mv->dst_y, mv->flags,
              //                                 mv->motion_x, mv->motion_y, mv->motion_scale));
           //   }
           // }
          //}}}
          char frameType = cDvbUtils::getFrameType (frame, frameSize, mH264);
          if (frameType == 'I') {
            mInterpolatedPts = dts;
            mGotIframe = true;
            }
          mRender.log ("pes", fmt::format ("pid:{} {} {} pts:{} dts:{} size:{}",
                                           pid, mStreamName, frameType,
                                           utils::getFullPtsString (pts), utils::getFullPtsString (dts),
                                           pesSize));
          // allocFrame
          cFFmpegVideoFrame* ffmpegVideoFrame = dynamic_cast<cFFmpegVideoFrame*>(allocFrameCallback());
          ffmpegVideoFrame->addTime (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count());
          ffmpegVideoFrame->setPts (mGotIframe ? mInterpolatedPts : dts);
          ffmpegVideoFrame->setPtsDuration ((kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num);
          ffmpegVideoFrame->setPesSize (frameSize);
          ffmpegVideoFrame->mFrameType = frameType;
          ffmpegVideoFrame->setAVFrame (avFrame);
          addFrameCallback (ffmpegVideoFrame);
          avFrame = av_frame_alloc();

          mInterpolatedPts += ffmpegVideoFrame->getPtsDuration();
          }
        }
      frame += bytesUsed;
      frameSize -= bytesUsed;
      }

    av_frame_unref (avFrame);
    av_frame_free (&avFrame);

    av_packet_free (&avPacket);
    return mInterpolatedPts;
    }
  //}}}

private:
  cRender& mRender;
  const uint8_t mStreamType;
  const bool mH264 = false;
  const std::string mStreamName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;

  bool mGotIframe = false;
  int64_t mInterpolatedPts = -1;
  };

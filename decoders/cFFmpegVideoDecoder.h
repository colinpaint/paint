// cFFmpegVideoDecoder.h
constexpr bool kMotionVectors = true;
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <algorithm>
#include <functional>

#include "cDecoder.h"
#include "cFFmpegVideoFrame.h"
//{{{  libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}

#include "../common/utils.h"
#include "../common/cDvbUtils.h"
#include "../common/cLog.h"
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
  cFFmpegVideoDecoder (bool h264) :
       cDecoder(), mH264(h264),
       mStreamName(h264 ? "h264" : "mpeg2"),
       mAvCodec(avcodec_find_decoder (h264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO)) {

    av_log_set_level (AV_LOG_ERROR);
    av_log_set_callback (logCallback);

    cLog::log (LOGINFO, fmt::format ("cFFmpegVideoDecoder - {}", mStreamName));

    mAvParser = av_parser_init (mH264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    mAvContext->flags2 |= AV_CODEC_FLAG2_FAST;

    if (kMotionVectors) {
      AVDictionary* opts = nullptr;
      av_dict_set (&opts, "flags2", "+export_mvs", 0);
      avcodec_open2 (mAvContext, mAvCodec, &opts);
      av_dict_free (&opts);
      }
    else
      avcodec_open2 (mAvContext, mAvCodec, nullptr);
    }
  //}}}
  //{{{
  virtual ~cFFmpegVideoDecoder() {

    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  virtual std::string getInfoString() const final { return mH264 ? "ffmpeg h264" : "ffmpeg mpeg"; }
  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool allocFront,
                          std::function<cFrame*(bool allocFront)> allocFrameCallback,
                          std::function<void (cFrame* frame)> addFrameCallback) final {
    (void)pts;

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

          char frameType = cDvbUtils::getFrameType (frame, frameSize, mH264);
          if (frameType == 'I') {
            mInterpolatedPts = dts;
            mGotIframe = true;
            }

          // allocFrame
          cFFmpegVideoFrame* ffmpegVideoFrame = dynamic_cast<cFFmpegVideoFrame*>(allocFrameCallback (allocFront));
          ffmpegVideoFrame->addTime (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count());
          ffmpegVideoFrame->set (mGotIframe ? mInterpolatedPts : dts,
                                 (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num,
                                 frameSize);
          ffmpegVideoFrame->mFrameType = frameType;
          ffmpegVideoFrame->setAVFrame (avFrame, kMotionVectors);
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
  const bool mH264 = false;
  const std::string mStreamName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;

  bool mGotIframe = false;
  int64_t mInterpolatedPts = -1;
  };

// cFFmpegVideoDecoder.h
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
#include "../common/cLog.h"

#include "../dvb/cDvbUtils.h"
//}}}
constexpr bool kMotionVectors = true;
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
       cDecoder(), mH264(h264), mStreamName(h264 ? "h264" : "mpeg2"),
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
  virtual void decode (uint8_t* pes, uint32_t pesSize, int64_t pts, const std::string& frameInfo,
                       std::function<cFrame*(int64_t pts)> allocFrameCallback,
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

          if (frameInfo.front() == 'I') {
            // pes come in dts order with repeated dts, pts jumps around
            // - ffmpeg decodes in pts order, some pes produce no frames, some produce more than one
            mSeqPts = pts;
            mGotIframe = true;
            }

          if (mGotIframe) {
            // alloc videoFrame
            cFFmpegVideoFrame* ffmpegVideoFrame = dynamic_cast<cFFmpegVideoFrame*>(allocFrameCallback (mSeqPts));
            if (ffmpegVideoFrame) {
              ffmpegVideoFrame->addTime (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count());
              ffmpegVideoFrame->set (mSeqPts,
                                     (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num,
                                     frameSize);
              ffmpegVideoFrame->mFrameType = frameInfo.front();
              ffmpegVideoFrame->setAVFrame (avFrame, kMotionVectors);

              // addFrame
              addFrameCallback (ffmpegVideoFrame);

              avFrame = av_frame_alloc();
              mSeqPts += ffmpegVideoFrame->getPtsDuration();
              }
            else
              mSeqPts += 3600;
            }
          else {
            // should we free pes here ???
            }
          }
        }
      frame += bytesUsed;
      frameSize -= bytesUsed;
      }

    av_frame_unref (avFrame);
    av_frame_free (&avFrame);

    av_packet_free (&avPacket);
    }
  //}}}

private:
  const bool mH264 = false;
  const std::string mStreamName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;

  bool mGotIframe = false;
  int64_t mSeqPts = -1;
  };

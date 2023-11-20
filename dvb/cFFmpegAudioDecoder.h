// cFFmpegAudioDecoder.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <algorithm>
#include <functional>

#include "../common/cLog.h"

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

#include "cAudioFrame.h"
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

class cFFmpegAudioDecoder : public cDecoder {
public:
  //{{{
  cFFmpegAudioDecoder (cRender& render, uint8_t streamType)
      : cDecoder(),
        mRender(render),
        mStreamType(streamType), mAacLatm(mStreamType == 17), mStreamTypeName(mAacLatm ? "aacL" : "mp3 "),
        mAvCodec(avcodec_find_decoder (mAacLatm ? AV_CODEC_ID_AAC_LATM : AV_CODEC_ID_MP3)) {

    av_log_set_level (AV_LOG_ERROR);
    av_log_set_callback (logCallback);

    cLog::log (LOGINFO, fmt::format ("cFFmpegAudioDecoder - streamType:{}:{}", mStreamType, mStreamTypeName));

    mAvParser = av_parser_init (mAacLatm ? AV_CODEC_ID_AAC_LATM : AV_CODEC_ID_MP3);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    mAvContext->flags2 |= AV_CODEC_FLAG2_FAST;

    avcodec_open2 (mAvContext, mAvCodec, NULL);
    }
  //}}}
  //{{{
  virtual ~cFFmpegAudioDecoder() {

    if (mAvContext)
      avcodec_close (mAvContext);

    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  virtual std::string getInfoString() const final { return "ffmpeg " + mStreamTypeName; }
  //{{{
  virtual int64_t decode (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          std::function<cFrame* ()> allocFrameCallback,
                          std::function<void (cFrame* frame)> addFrameCallback) final  {
    (void)dts;

    mRender.log ("pes", fmt::format ("pid:{} pts:{} size:{}",
                                     pid, utils::getFullPtsString (pts), pesSize));

    AVPacket* avPacket = av_packet_alloc();
    AVFrame* avFrame = av_frame_alloc();

    int64_t interpolatedPts = pts;
    uint8_t* pesPtr = pes;
    while (pesSize) {
      // parse pesFrame, pes may have more than one frame
      auto now = std::chrono::system_clock::now();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                        pesPtr, (int)pesSize, 0, 0, AV_NOPTS_VALUE);
      pesPtr += bytesUsed;
      pesSize -= bytesUsed;
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;

          if (avFrame->nb_samples > 0) {
            // call allocAudioFrame callback
            cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(allocFrameCallback());
            audioFrame->addTime (std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - now).count());
            audioFrame->mPts = interpolatedPts;
            audioFrame->mPtsDuration = avFrame->sample_rate ? (avFrame->nb_samples * 90000 / avFrame->sample_rate) : 48000;
            audioFrame->mPesSize = bytesUsed;
            audioFrame->mSamplesPerFrame = avFrame->nb_samples;
            audioFrame->mSampleRate = avFrame->sample_rate;
            audioFrame->mNumChannels = avFrame->ch_layout.nb_channels;

            float* dst = audioFrame->mSamples.data();
            switch (mAvContext->sample_fmt) {
              case AV_SAMPLE_FMT_FLTP:
                // 32bit float planar, copy to interleaved
                for (int sample = 0; sample < avFrame->nb_samples; sample++)
                  for (int channel = 0; channel < avFrame->ch_layout.nb_channels; channel++)
                    *dst++ = *(((float*)avFrame->data[channel]) + sample);
                break;
              case AV_SAMPLE_FMT_S16P:
                // 16bit signed planar, scale to interleaved
                for (int sample = 0; sample < avFrame->nb_samples; sample++)
                  for (int channel = 0; channel < avFrame->ch_layout.nb_channels; channel++)
                    *dst++ = (*(((short*)avFrame->data[channel]) + sample)) / (float)0x8000;
                break;
              default:;
              }
            audioFrame->calcPower();

            // call addFrame callback
            addFrameCallback (audioFrame);

            // next pts
            interpolatedPts += audioFrame->mPtsDuration;
            }
          }
        }
      av_frame_unref (avFrame);
      }

    av_frame_free (&avFrame);
    av_packet_free (&avPacket);
    return interpolatedPts;
    }
  //}}}

private:
  cRender& mRender;
  const uint8_t mStreamType;
  const bool mAacLatm;
  const std::string mStreamTypeName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;
  };

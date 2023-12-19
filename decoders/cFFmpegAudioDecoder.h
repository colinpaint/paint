// cFFmpegAudioDecoder.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <algorithm>
#include <functional>

#include "cDecoder.h"
#include "cAudioParser.h"
#include "cAudioFrame.h"
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

#include "../common/cLog.h"
//}}}
constexpr bool kDebug = false;

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
  cFFmpegAudioDecoder (eAudioFrameType audioFrameType) :
      cDecoder(),
      mAudioFrameType(audioFrameType),
      mStreamTypeName((audioFrameType == eAudioFrameType::eAacLatm) ? "aacL" :
                        (audioFrameType == eAudioFrameType::eAacAdts) ? "aacA" : "mp3 "),
      mAvCodec(avcodec_find_decoder ((audioFrameType == eAudioFrameType::eAacLatm) ? AV_CODEC_ID_AAC_LATM :
                                       (audioFrameType == eAudioFrameType::eAacAdts) ? AV_CODEC_ID_AAC: AV_CODEC_ID_MP3)) {

    av_log_set_level (AV_LOG_ERROR);
    av_log_set_callback (logCallback);

    cLog::log (LOGINFO, fmt::format ("cFFmpegAudioDecoder {}", mStreamTypeName));

    mAvParser = av_parser_init ((audioFrameType == eAudioFrameType::eAacLatm) ? AV_CODEC_ID_AAC_LATM :
                                  (audioFrameType == eAudioFrameType::eAacAdts) ? AV_CODEC_ID_AAC: AV_CODEC_ID_MP3);
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

  size_t getNumChannels() const { return mChannels; }
  size_t getSampleRate() const { return mSampleRate; }
  size_t getNumSamplesPerFrame() const { return mSamplesPerFrame; }

  virtual std::string getInfoString() const final { return "ffmpeg " + mStreamTypeName; }
  //{{{
  virtual int64_t decode (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          std::function<cFrame* ()> allocFrameCallback,
                          std::function<void (cFrame* frame)> addFrameCallback) final  {
    (void)pid;
    (void)dts;

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
            audioFrame->setPts (interpolatedPts);
            audioFrame->setPtsDuration (avFrame->sample_rate ? (avFrame->nb_samples * 90000 / avFrame->sample_rate) : 48000);
            audioFrame->setPesSize (bytesUsed);
            audioFrame->setSamplesPerFrame (avFrame->nb_samples);
            audioFrame->setSampleRate (avFrame->sample_rate);
            audioFrame->setNumChannels (avFrame->ch_layout.nb_channels);

            if (kDebug)
              cLog::log (LOGINFO, fmt::format ("pts:{} {} chans:{}:{}:{} {} {}",
                                               utils::getFullPtsString (pts),
                                               utils::getFullPtsString (interpolatedPts),
                                               audioFrame->getNumChannels(),
                                               audioFrame->getSampleRate(),
                                               audioFrame->getSamplesPerFrame(),
                                               audioFrame->getPtsDuration(),
                                               pesSize));
            float* dst = audioFrame->mSamples.data();
            switch (mAvContext->sample_fmt) {
              case AV_SAMPLE_FMT_FLTP:
                //{{{  32bit float planar, copy to interleaved
                for (int sample = 0; sample < avFrame->nb_samples; sample++)
                  for (int channel = 0; channel < avFrame->ch_layout.nb_channels; channel++)
                    *dst++ = *(((float*)avFrame->data[channel]) + sample);
                break;
                //}}}
              case AV_SAMPLE_FMT_S16P:
                //{{{  16bit signed planar, scale to interleaved
                for (int sample = 0; sample < avFrame->nb_samples; sample++)
                  for (int channel = 0; channel < avFrame->ch_layout.nb_channels; channel++)
                    *dst++ = (*(((short*)avFrame->data[channel]) + sample)) / (float)0x8000;
                break;
                //}}}
              default:;
              }
            addFrameCallback (audioFrame);

            interpolatedPts += audioFrame->getPtsDuration();
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

  //{{{
  float* decodeFrame (const uint8_t* framePtr, int frameLen, int64_t pts) {
  // return frame decoded to a buffer of floats

    float* samples = nullptr;

    AVPacket* avPacket = av_packet_alloc();
    AVFrame* avFrame = av_frame_alloc();

    auto pesPtr = framePtr;
    auto pesSize = frameLen;
    while (pesSize) {
      auto bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                         pesPtr, (int)pesSize, pts, AV_NOPTS_VALUE, 0);
      avPacket->pts = pts;
      pesPtr += bytesUsed;
      pesSize -= bytesUsed;
      if (avPacket->size) {
        auto ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
            break;

          if (avFrame->nb_samples > 0) {
            switch (mAvContext->sample_fmt) {
              //{{{
              case AV_SAMPLE_FMT_FLTP: { // 32bit float planar, copy to interleaved, mix down 5.1
                mChannels = 2;
                mSampleRate = avFrame->sample_rate;
                mSamplesPerFrame = avFrame->nb_samples;

                samples = (float*)malloc (mChannels * mSamplesPerFrame * sizeof(float));
                float* dstPtr = samples;

                float* srcPtr0 = (float*)avFrame->data[0];
                float* srcPtr1 = (float*)avFrame->data[1];
                if (avFrame->ch_layout.nb_channels == 6) {
                  // 5.1
                  float* srcPtr2 = (float*)avFrame->data[2];
                  float* srcPtr3 = (float*)avFrame->data[3];
                  float* srcPtr4 = (float*)avFrame->data[4];
                  float* srcPtr5 = (float*)avFrame->data[5];
                  for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
                    *dstPtr++ = *srcPtr0++ + *srcPtr2++ + *srcPtr4 + *srcPtr5; // left loud
                    *dstPtr++ = *srcPtr1++ + *srcPtr3++ + *srcPtr4++ + *srcPtr5++; // right loud
                    }
                  }
                else {
                  // stereo
                  for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
                    *dstPtr++ = *srcPtr0++;
                    *dstPtr++ = *srcPtr1++;
                    }
                  }
                break;
                }
              //}}}
              //{{{
              case AV_SAMPLE_FMT_S16P: // 16bit signed planar, copy scale and copy to interleaved
                mChannels =  avFrame->ch_layout.nb_channels;
                mSampleRate = avFrame->sample_rate;
                mSamplesPerFrame = avFrame->nb_samples;

                samples = (float*)malloc (avFrame->ch_layout.nb_channels * avFrame->nb_samples * sizeof(float));

                for (size_t channel = 0; channel < (size_t)(avFrame->ch_layout.nb_channels); channel++) {
                  float* dstPtr = samples + channel;
                  short* srcPtr = (short*)avFrame->data[channel];
                  for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
                    *dstPtr = *srcPtr++ / (float)0x8000;
                    dstPtr += mChannels;
                    }
                  }
                break;
              //}}}
              default:;
              }
            }
          av_frame_unref (avFrame);
          }
        }
      }

    av_frame_free (&avFrame);
    av_packet_free (&avPacket);

    return samples;
    }
  //}}}

private:
  const eAudioFrameType mAudioFrameType;
  const std::string mStreamTypeName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;

  size_t mChannels = 0;
  size_t mSampleRate = 0;
  size_t mSamplesPerFrame = 0;
  };
// cFFmpegAudioDecoder.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <chrono>

#include "cFFmpegAudioDecoder.h"

#include "../utils/utils.h"
#include "../utils/cLog.h"

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

using namespace std;
using namespace chrono;
//}}}

//{{{
cFFmpegAudioDecoder::cFFmpegAudioDecoder (eAudioFrameType frameType) {

  AVCodecID streamType;

  switch (frameType) {
    case eAudioFrameType::eMp3:
      streamType =  AV_CODEC_ID_MP3;
      break;

    case eAudioFrameType::eAacAdts:
      streamType =  AV_CODEC_ID_AAC;
      break;

    case eAudioFrameType::eAacLatm:
      streamType =  AV_CODEC_ID_AAC_LATM;
      break;

    default:
      cLog::log (LOGERROR, "unknown cFFmpegAacDecoder frameType %d", frameType);
      return;
    }

  mAvParser = av_parser_init (streamType);
  mAvCodec = (AVCodec*)avcodec_find_decoder (streamType);
  mAvContext = avcodec_alloc_context3 (mAvCodec);
  avcodec_open2 (mAvContext, mAvCodec, NULL);
  }
//}}}
//{{{
cFFmpegAudioDecoder::~cFFmpegAudioDecoder() {

  if (mAvContext)
    avcodec_close (mAvContext);
  if (mAvParser)
    av_parser_close (mAvParser);
  }
//}}}

//{{{
float* cFFmpegAudioDecoder::decodeFrame (const uint8_t* framePtr, int frameLen, int64_t pts) {

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
              if (avFrame->ch_layout.nb_channels == 6) { // 5.1
                float* srcPtr2 = (float*)avFrame->data[2];
                float* srcPtr3 = (float*)avFrame->data[3];
                float* srcPtr4 = (float*)avFrame->data[4];
                float* srcPtr5 = (float*)avFrame->data[5];
                for (int sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++ + *srcPtr2++ + *srcPtr4 + *srcPtr5; // left loud
                  *dstPtr++ = *srcPtr1++ + *srcPtr3++ + *srcPtr4++ + *srcPtr5++; // right loud
                  }
                }
              else // stereo
                for (int sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++;
                  *dstPtr++ = *srcPtr1++;
                  }
                }
              break;
            //}}}
            //{{{
            case AV_SAMPLE_FMT_S16P: // 16bit signed planar, copy scale and copy to interleaved
              mChannels =  avFrame->ch_layout.nb_channels;
              mSampleRate = avFrame->sample_rate;
              mSamplesPerFrame = avFrame->nb_samples;
              samples = (float*)malloc (avFrame->ch_layout.nb_channels * avFrame->nb_samples * sizeof(float));

              for (int channel = 0; channel < avFrame->ch_layout.nb_channels; channel++) {
                float* dstPtr = samples + channel;
                short* srcPtr = (short*)avFrame->data[channel];
                for (int sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr = *srcPtr++ / (float)0x8000;
                  dstPtr += mChannels;
                  }
                }
              break;
            //}}}
            default:;
            }

          }
        //av_frame_unref (avFrame);
        }
      }
    }

  mLastPts = pts;

  av_frame_free (&avFrame);
  av_packet_free (&avPacket);
  return samples;
  }
//}}}

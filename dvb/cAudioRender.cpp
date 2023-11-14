// cAudioRender.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

#include "cAudioRender.h"
#include "cAudioFrame.h"

#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

#include <cstdint>
#include <string>
#include <array>
#include <algorithm>
#include <functional>
#include <thread>

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/utils.h"

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

using namespace std;
//}}}
constexpr bool kAudioQueued = true;
constexpr size_t kSamplesWait = 2;
constexpr size_t kAudioFrameMapSize = 48;

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

//{{{
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

  virtual string getInfoString() const final { return "ffmpeg " + mStreamTypeName; }
  //{{{
  virtual int64_t decode (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          function<cFrame* ()> allocFrameCallback,
                          function<void (cFrame* frame)> addFrameCallback) final  {
    (void)dts;

    mRender.log ("pes", fmt::format ("pid:{} pts:{} size:{}",
                                     pid, utils::getFullPtsString (pts), pesSize));

    AVPacket* avPacket = av_packet_alloc();
    AVFrame* avFrame = av_frame_alloc();

    int64_t interpolatedPts = pts;
    uint8_t* pesPtr = pes;
    while (pesSize) {
      // parse pesFrame, pes may have more than one frame
      auto now = chrono::system_clock::now();
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
            audioFrame->addTime (chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - now).count());
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
  const string mStreamTypeName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;
  };
//}}}

// cAudioRender
//{{{
cAudioRender::cAudioRender (const string& name, uint8_t streamType)
    : cRender(kAudioQueued, name + "aud", streamType, kAudioFrameMapSize),
      mNumChannels(2), mSampleRate(48000), mSamplesPerFrame(1024), mPtsDuration(0) {

  mDecoder = new cFFmpegAudioDecoder (*this, streamType);

  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });

  mPlayerThread = thread ([=]() {
    cLog::setThreadName ("play");
    array <float,2048*2> samples = { 0.f };
    array <float,2048*2> silence = { 0.f };

    #ifdef _WIN32
      //{{{  windows
      optional<cAudioDevice> audioDevice = getDefaultAudioOutputDevice();
      if (audioDevice) {
        cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", mSampleRate);

        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        audioDevice->setSampleRate (mSampleRate);
        audioDevice->start();

        int samplesWait = kSamplesWait;
        while (!mExit) {
          audioDevice->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
            // process loadSrcSamples callback lambda
            srcSamples = silence.data();

            // locked audio mutex
            shared_lock<shared_mutex> lock (getSharedMutex());
            cAudioFrame* audioFrame = findAudioFrameFromPts (mPlayerPts);
            if (mPlaying && audioFrame && audioFrame->mSamples.data()) {
              samplesWait = 0;
              if (!mMute) {
                float* src = audioFrame->mSamples.data();
                float* dst = samples.data();
                switch (audioFrame->mNumChannels) {
                  //{{{
                  case 1: // mono to 2 interleaved channels
                    for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                      *dst++ = *src;
                      *dst++ = *src++;
                      }
                    break;
                  //}}}
                  //{{{
                  case 2: // stereo to 2 interleaved channels
                    memcpy (dst, src, audioFrame->mSamplesPerFrame * audioFrame->mNumChannels * sizeof(float));
                    break;
                  //}}}
                  //{{{
                  case 6: // 5.1 to 2 interleaved channels
                    for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                      *dst++ = src[0] + src[2] + src[4] + src[5]; // left loud
                      *dst++ = src[1] + src[3] + src[4] + src[5]; // right loud
                      src += 6;
                      }
                    break;
                  //}}}
                  //{{{
                  default:
                    cLog::log (LOGERROR, fmt::format ("cAudioPlayer unknown num channels {}", audioFrame->mNumChannels));
                  //}}}
                  }
                srcSamples = samples.data();
                }
              }
            else
              samplesWait--;
            numSrcSamples = (int)(audioFrame ? audioFrame->mSamplesPerFrame : mSamplesPerFrame);

            if (mPlaying)
              if (!samplesWait) {
                mPlayerPts += audioFrame ? audioFrame->mPtsDuration : mPtsDuration;
                samplesWait = kSamplesWait;
                }
            });
          }

        audioDevice->stop();
        }
      //}}}
    #else
      //{{{  linux
      // raise to max priority
      cAudio audio (2, mSampleRate, 48000, false);

      sched_param sch_params;
      sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
      pthread_setschedparam (mPlayerThread.native_handle(), SCHED_RR, &sch_params);

      int samplesWait = kSamplesWait;
      while (!mExit) {
        float* srcSamples = silence.data();
        cAudioFrame* audioFrame;

        { // locked mutex
        shared_lock<shared_mutex> lock (getSharedMutex());
        audioFrame = findAudioFrameFromPts (mPlayerPts);
        if (mPlaying && audioFrame && audioFrame->mSamples.data()) {
          samplesWait = 0;
          if (!mMute) {
            float* src = audioFrame->mSamples.data();
            float* dst = samples.data();
            switch (audioFrame->mNumChannels) {
              //{{{
              case 1: // mono to 2 interleaved channels
                for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                break;
              //}}}
              //{{{
              case 2: // stereo to 2 interleaved channels
                memcpy (dst, src, audioFrame->mSamplesPerFrame * 8);
                break;
              //}}}
              //{{{
              case 6: // 5.1 to 2 interleaved channels
                for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                  *dst++ = src[0] + src[2] + src[4] + src[5]; // left loud
                  *dst++ = src[1] + src[3] + src[4] + src[5]; // right loud
                  src += 6;
                  }
                break;
              //}}}
              //{{{
              default:
                cLog::log (LOGERROR, fmt::format ("cAudioPlayer unknown num channels {}", audioFrame->mNumChannels));
              //}}}
              }
            srcSamples = samples.data();
            }
          }
        else
          samplesWait--;
        }

        audio.play (2, srcSamples, audioFrame ? audioFrame->mSamplesPerFrame : mSamplesPerFrame, 1.f);

        if (mPlaying)
          if (!samplesWait) {
            mPlayerPts += audioFrame ? audioFrame->mPtsDuration : mPtsDuration;
            samplesWait = kSamplesWait;
            }
        }
      //}}}

    #endif

    mRunning = false;
    cLog::log (LOGINFO, "exit");
    });

  mPlayerThread.detach();
  }
//}}}
//{{{
cAudioRender::~cAudioRender() {

  exitWait();

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFrames)
    delete (frame.second);
  mFrames.clear();

  for (auto& frame : mFreeFrames)
    delete frame;
  mFreeFrames.clear();
  }
//}}}

//{{{
cAudioFrame* cAudioRender::findAudioFrameFromPts (int64_t pts) {
  return dynamic_cast<cAudioFrame*>(getFrameFromPts (pts));
  }
//}}}

// decoder callbacks
//{{{
cFrame* cAudioRender::getFrame() {

  cFrame* frame = getFreeFrame();
  if (frame)
    return frame;

  return new cAudioFrame();
  }
//}}}
//{{{
void cAudioRender::addFrame (cFrame* frame) {

  cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);

  mNumChannels = audioFrame->mNumChannels;
  mSampleRate = audioFrame->mSampleRate;
  mSamplesPerFrame = audioFrame->mSamplesPerFrame;
  mPtsDuration = audioFrame->mPtsDuration;
  mFrameInfo = audioFrame->getInfoString();

  logValue (audioFrame->mPts, audioFrame->mPowerValues[0]);

  { // locked emplace
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFrames.emplace (audioFrame->mPts, audioFrame);
  }

  // start player
  if (!isPlaying())
    startPlayerPts (audioFrame->mPts);
  }
//}}}

// overrides
//{{{
string cAudioRender::getInfoString() const {
  return fmt::format ("aud frames:{:2d}:{:2d}:{:d} {} {}",
                      mFrames.size(), mFreeFrames.size(), getQueueSize(),
                      mDecoder->getInfoString(), mFrameInfo);
  }
//}}}
//{{{
bool cAudioRender::processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {

  // throttle on number of decoded audioFrames
  while (mFrames.size() > mFrameMapSize) {
    this_thread::sleep_for (1ms);
    trimFramesBeforePts (getPlayerPts());
    }

  return cRender::processPes (pid, pes, pesSize, pts, dts, skip);
  }
//}}}

// private
//{{{
void cAudioRender::startPlayerPts (int64_t pts) {

  mPlayerPts = pts;
  mPlaying = true;
  }
//}}}
//{{{
void cAudioRender::exitWait() {

  mExit = true;
  while (mRunning)
    this_thread::sleep_for (100ms);
  }
//}}}

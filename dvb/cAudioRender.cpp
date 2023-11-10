// cAudioRender.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cAudioRender.h"
#include "cAudioFrame.h"

#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
#endif

#ifdef __linux__
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
constexpr int kAudioPlayerPreloadFrames = 16;

//{{{
class cFFmpegAudioDecoder : public cDecoder {
public:
  //{{{
  cFFmpegAudioDecoder (cRender* render, uint8_t streamType)
    : cDecoder(),
      mRender(render),
      mStreamType(streamType),
      mAacLatm(mStreamType == 17),
      mStreamTypeName(mAacLatm ? "aacL" : "mp3 "),
      mAvCodec(avcodec_find_decoder (mAacLatm ? AV_CODEC_ID_AAC_LATM : AV_CODEC_ID_MP3)) {

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
    (void)pid;
    (void)dts;

    AVPacket* avPacket = av_packet_alloc();
    AVFrame* avFrame = av_frame_alloc();

    int64_t interpolatedPts = pts;

    // parse pesFrame, pes may have more than one frame
    uint8_t* pesPtr = pes;

    mRender->log ("pes", fmt::format ("pts:{} size: {}", utils::getFullPtsString (pts), pesSize));
    mRender->logValue (pts, (float)pesSize);

    while (pesSize) {
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
  cRender* mRender;
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
cAudioRender::cAudioRender (const string& name, uint8_t streamType, uint16_t decoderMask)
    : cRender(kAudioQueued, name + "aud", streamType, decoderMask, kAudioFrameMapSize),
      mNumChannels(2), mSampleRate(48000), mSamplesPerFrame(1024),
      mPtsDuration(0), mPlayerFrames(0) {

  mDecoder = new cFFmpegAudioDecoder (this, streamType);

  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });

  #ifdef _WIN32
    //{{{
    mPlayerThread = thread ([=]() {
      cLog::setThreadName ("play");
      SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

      optional<cAudioDevice> audioDevice = getDefaultAudioOutputDevice();
      if (audioDevice) {
        cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", mSampleRate);
        audioDevice->setSampleRate (mSampleRate);
        audioDevice->start();

        array <float,2048*2> samples = { 0.f };
        array <float,2048*2> silence = { 0.f };
        while (!mExit) {
          audioDevice->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
            // loadSrcSamples callback lambda
            // locked audio mutex
            shared_lock<shared_mutex> lock (getSharedMutex());
            cAudioFrame* audioFrame = getAudioFrameFromPts (mPlayerPts);
            if (mPlaying && audioFrame) {
              float* src = audioFrame->mSamples.data();
              float* dst = samples.data();
              switch (audioFrame->mNumChannels) {
                //{{{
                case 1: // mono to 2 channels
                  for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                    *dst++ = *src;
                    *dst++ = *src++;
                    }
                  break;
                //}}}
                //{{{
                case 2: // stereo
                  memcpy (dst, src, audioFrame->mSamplesPerFrame * audioFrame->mNumChannels * sizeof(float));
                  break;
                //}}}
                //{{{
                case 6: // 5.1 to 2 channels
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
            else
              srcSamples = silence.data();
            numSrcSamples = (int)(audioFrame ? audioFrame->mSamplesPerFrame : mSamplesPerFrame);

            if (mPlaying)
              mPlayerPts += audioFrame ? audioFrame->mPtsDuration : mPtsDuration;
            });
          }

        audioDevice->stop();
        }

      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });

    mPlayerThread.detach();
    //}}}
  #else
    //{{{
    mPlayerThread = thread ([=]() {
      // lambda
      cLog::setThreadName ("play");

      // raise to max priority
      sched_param sch_params;
      sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
      pthread_setschedparam (mPlayerThread.native_handle(), SCHED_RR, &sch_params);

      cAudio audio (2, mSampleRate, 40000, false);

      array <float,2048*2> samples = { 0.f };
      array <float,2048*2> silence = { 0.f };

      while (!mExit) {
        float* srcSamples = silence.data();

        cAudioFrame* audioFrame;
        { // locked mutex
        shared_lock<shared_mutex> lock (getSharedMutex());
        audioFrame = getAudioFrameFromPts (mPlayerPts);
        if (mPlaying && audioFrame && audioFrame->mSamples.data()) {
          float* src = audioFrame->mSamples.data();
          float* dst = samples.data();
          switch (audioFrame->mNumChannels) {
            //{{{
            case 1: // mono to 2 channels
              for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                *dst++ = *src;
                *dst++ = *src++;
                }
              break;
            //}}}
            //{{{
            case 2: // stereo
              memcpy (dst, src, audioFrame->mSamplesPerFrame * 8);
              break;
            //}}}
            //{{{
            case 6: // 5.1 to 2 channels
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

        audio.play (2, srcSamples,
                    audioFrame ? audioFrame->mSamplesPerFrame : mSamplesPerFrame,
                    1.f);

        if (mPlaying)
          mPlayerPts += audioFrame ? audioFrame->mPtsDuration : mPtsDuration;
        }

      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });

    mPlayerThread.detach();
    //}}}
  #endif
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
void cAudioRender::startPlayerPts (int64_t pts) {
  mPlayerPts = pts;
  mPlaying = true;
  }
//}}}

cAudioFrame* cAudioRender::getAudioFrameFromPts (int64_t pts) {
  return dynamic_cast<cAudioFrame*>(getFrameFromPts (pts));
  }

// callbacks
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

  // start player after kPlayerPreloadFrames of audio decoded
  mPlayerFrames++;
  if (!isPlaying())
    if (mPlayerFrames == kAudioPlayerPreloadFrames)
      startPlayerPts (audioFrame->mPts - (kAudioPlayerPreloadFrames * audioFrame->mPtsDuration));
  }
//}}}

//{{{
string cAudioRender::getInfoString() const {
  return fmt::format ("aud frames:{:2d}:{:2d}:{:d} {} {}",
                      mFrames.size(), mFreeFrames.size(), getQueueSize(),
                      mDecoder->getInfoString(), mFrameInfo);
  }
//}}}
//{{{
bool cAudioRender::processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {

  // throttle on number of queued audioFrames
  while (mFrames.size() >= mFrameMapSize) {
    this_thread::sleep_for (1ms);
    trimFramesBeforePts (getPlayerPts());
    }

  return cRender::processPes (pid, pes, pesSize, pts, dts, skip);
  }
//}}}

//{{{
void cAudioRender::exitWait() {
  mExit = true;
  while (mRunning)
    this_thread::sleep_for (100ms);
  }
//}}}

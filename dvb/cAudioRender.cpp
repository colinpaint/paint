// cAudioRender.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

#include "cAudioRender.h"

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

#include "cFFmpegAudioDecoder.h"

using namespace std;
//}}}
constexpr bool kAudioQueued = true;
constexpr size_t kAudioFrameMapSize = 24;
constexpr int kSamplesWait = 2;

// cAudioRender
//{{{
cAudioRender::cAudioRender (const string& name, uint8_t streamType, bool realTime)
    : cRender(kAudioQueued, name + "aud", streamType, kAudioFrameMapSize, realTime),
      mSampleRate(48000), mSamplesPerFrame(1024), mPtsDuration(0) {

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
                  case 1: // interleaved mono to 2 interleaved 2 channels
                    for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                      *dst++ = *src;
                      *dst++ = *src++;
                      }
                    break;
                  //}}}
                  //{{{
                  case 2: // interleaved stereo to 2 interleaved 2 channels
                    memcpy (dst, src, audioFrame->mSamplesPerFrame * 8);
                    break;
                  //}}}
                  //{{{
                  case 6: // interleaved 5.1 to 2 interleaved 2channels
                    for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                      *dst++ = src[0] + src[2] + src[3] + src[4]; // left
                      *dst++ = src[1] + src[2] + src[3] + src[5]; // right
                      src += 6;
                      }
                    break;
                  //}}}
                  //{{{
                  default:
                    cLog::log (LOGERROR, fmt::format ("cAudioPlayer unknown num channels {}",
                                                      audioFrame->mNumChannels));
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
              case 1: // interleaved mono to 2 interleaved 2 channels
                for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                break;
              //}}}
              //{{{
              case 2: // interleaved stereo to 2 interleaved 2 channels
                memcpy (dst, src, audioFrame->mSamplesPerFrame * 8);
                break;
              //}}}
              //{{{
              case 6: // interleaved 5.1 to 2 interleaved 2channels
                for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                  *dst++ = src[0] + src[2] + src[3] + src[4]; // left
                  *dst++ = src[1] + src[2] + src[3] + src[5]; // right
                  src += 6;
                  }
                break;
              //}}}
              //{{{
              default:
                cLog::log (LOGERROR, fmt::format ("cAudioPlayer unknown num channels {}",
                                                  audioFrame->mNumChannels));
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

  mExit = true;
  while (mRunning)
    this_thread::sleep_for (10ms);

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

  if (mSampleRate != audioFrame->mSampleRate) {
    cLog::log (LOGERROR, fmt::format ("cAudioRender::addFrame sampleRate changed from {} to {}",
                                      mSampleRate, audioFrame->mSampleRate));
    mSampleRate = audioFrame->mSampleRate;
    }

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

  trimFramesBeforePts (getPlayerPts() - mPtsDuration);

  if (!getRealTime())
    // throttle on number of decoded audioFrames
    while (mFrames.size() > mFrameMapSize) {
      this_thread::sleep_for (1ms);
      trimFramesBeforePts (getPlayerPts() - mPtsDuration);
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

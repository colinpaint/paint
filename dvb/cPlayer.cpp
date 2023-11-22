// cPlayer.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

#include "cPlayer.h"

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

#include "../common/cLog.h"
#include "../common/utils.h"

#include "cAudioFrame.h"
#include "cAudioRender.h"

using namespace std;
//}}}
constexpr int kSamplesWait = 2;

//{{{
cPlayer::cPlayer (cAudioRender& audioRender, uint32_t sampleRate) 
    : mAudioRender(audioRender), mSampleRate(sampleRate) {

  mPlayerThread = thread ([=]() {

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
            shared_lock<shared_mutex> lock (mAudioRender.getSharedMutex());
            cAudioFrame* audioFrame = mAudioRender.findAudioFrameFromPts (mPts);
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
            numSrcSamples = (int)(audioFrame ? audioFrame->mSamplesPerFrame : mAudioRender.getSamplesPerFrame());

            if (mPlaying)
              if (!samplesWait) {
                mPts += audioFrame ? audioFrame->mPtsDuration : mAudioRender.getPtsDuration();
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
      cAudio audio (2, mSampleRate, 4096, false);

      sched_param sch_params;
      sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
      pthread_setschedparam (mPlayerThread.native_handle(), SCHED_RR, &sch_params);

      int samplesWait = kSamplesWait;
      while (!mExit) {
        float* srcSamples = silence.data();
        cAudioFrame* audioFrame;

        { // locked mutex
        shared_lock<shared_mutex> lock (getSharedMutex());
        audioFrame = mAudioRender.findAudioFrameFromPts (mPts);
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

        audio.play (2, srcSamples, audioFrame ? audioFrame->mSamplesPerFrame : mAudioRender.getSamplesPerFrame(), 1.f);

        if (mPlaying)
          if (!samplesWait) {
            mPts += audioFrame ? audioFrame->mPtsDuration : mAudioRender.getPtsDuration();
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
cPlayer::~cPlayer() {

  mExit = true;
  while (mRunning)
    this_thread::sleep_for (10ms);
  }
//}}}

//{{{
void cPlayer::startPts (int64_t pts) {

  mPts = pts;
  mPlaying = true;
  }
//}}}

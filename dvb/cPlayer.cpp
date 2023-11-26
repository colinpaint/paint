// cPlayer.cpp                                                     utils::getFullPtsString (pts)
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
namespace {
  array <float, 2048*2> gSilence = { 0.f };
  }

cPlayer::cPlayer (cAudioRender& audioRender, uint32_t sampleRate)
    : mAudioRender(audioRender), mSampleRate(sampleRate) {

  mPlayerThread = thread ([=]() {
    cLog::setThreadName ("play");

    array <float, 2048*2> samples = { 0.f };
    //{{{  startup
    #ifdef _WIN32
      // windows
      optional<cAudioDevice> audioDevice = getDefaultAudioOutputDevice();
      if (!audioDevice) {
        cLog::log (LOGERROR, fmt::format ("- failed to create WASPI device"));
        return;
        }
      //cLog::log (LOGINFO, fmt::format ("created windows WASPI device {}hz", mSampleRate));

      SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
      audioDevice->setSampleRate (mSampleRate);
      audioDevice->start();

    #else
      // linux
      cAudio audio (2, mSampleRate, 4096, false);

      // raise to max priority
      sched_param sch_params;
      sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
      pthread_setschedparam (mPlayerThread.native_handle(), SCHED_RR, &sch_params);

    #endif
    //}}}

    while (!mExit) {
      #ifdef _WIN32
        audioDevice->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
      #else
        float* srcSamples;
        int numSrcSamples;
      #endif

      srcSamples = gSilence.data();
      numSrcSamples = (int)mAudioRender.getSamplesPerFrame();
      int64_t frameDuration = mAudioRender.getPtsDuration();

      if (mPlaying) {
        cAudioFrame* audioFrame = mAudioRender.getAudioFrameFromPts (mPts);
        if (audioFrame && audioFrame->mSamples.data()) {
          if (!mMute) {
            float* src = audioFrame->mSamples.data();
            float* dst = samples.data();
            switch (audioFrame->mNumChannels) {
              //{{{
              case 1: // mono to 2 interleaved 2 channels
                for (size_t i = 0; i < audioFrame->mSamplesPerFrame; i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                break;
              //}}}
              //{{{
              case 2: // interleaved stereo to 2 interleaved channels
                memcpy (dst, src, audioFrame->mSamplesPerFrame * 8);
                break;
              //}}}
              //{{{
              case 6: // interleaved 5.1 to 2 interleaved channels
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
          frameDuration = audioFrame->mPtsDuration;
          numSrcSamples = (int)audioFrame->mSamplesPerFrame;
          }
        else
          mMissedFrames++;
        }
      //{{{  linux play srcSamples
      #ifndef _WIN32
        audio.play (2, srcSamples, numSrcSamples, 1.f);
      #endif
      //}}}
      mAudioRender.audioTrimBeforePts (mPts);

      if (mPlaying) {
        if (mMissedFrames > 0) {
          cLog::log (LOGINFO, fmt::format ("cPlayer missed {}", utils::getFullPtsString (mPts)));
          mMissedFrames = 0;
          }
        else
          mPts += frameDuration;
        }
      //{{{  close windows play lamda
      #ifdef _WIN32
        });
      #endif
      //}}}
      }

    //{{{  close audioDevice
    #ifdef _WIN32
      audioDevice->stop();
    #endif
    //}}}
    mRunning = false;
    cLog::log (LOGINFO, "exit");
    });

  mPlayerThread.detach();
  }

cPlayer::~cPlayer() {
  mExit = true;
  while (mRunning)
    this_thread::sleep_for (1ms);
  }

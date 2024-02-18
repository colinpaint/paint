// cAudioPlayer.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

#include "cAudioPlayer.h"

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

cAudioPlayer::cAudioPlayer (cAudioRender& audioRender, uint32_t sampleRate, uint16_t pid, bool hasAudio) :
    mAudioRender(audioRender), mSampleRate(sampleRate) {

  (void)hasAudio;
  mPlayerThread = thread ([=]() {
    cLog::setThreadName (fmt::format ("{:4d}", pid));
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
      cAudio audio (2, mSampleRate, 4096);

      // raise to max priority
      sched_param sch_params;
      sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
      pthread_setschedparam (mPlayerThread.native_handle(), SCHED_RR, &sch_params);
    #endif
    //}}}

    while (!mExit) {
      #ifdef _WIN32
        audioDevice->process ([&](float*& srcSamples, int& numSamples) mutable noexcept {
      #else
        float* srcSamples;
        int numSamples;
      #endif

      srcSamples = mSilenceSamples.data();
      numSamples = (int)mAudioRender.getSamplesPerFrame();
      int64_t ptsDuration = 0;

      if (mPlaying || mSkip) {
        cAudioFrame* audioFrame = mAudioRender.getAudioFrameAtPts (mPts);
        if (audioFrame) {
          mSynced = true;
          mSkip = false;
          }
        else if (mPlaying) {
          // skip to nextFrame
          audioFrame = mAudioRender.getAudioFrameAtOrAfterPts (mPts);
          if (audioFrame) {// found it, adjust pts
            mPts = audioFrame->getPts();
            //cLog::log (LOGINFO, fmt::format ("skipped {}", utils::getFullPtsString (mPts)));
            }
          if (!audioFrame && mSynced)
            cLog::log (LOGINFO, fmt::format ("play missed:{}", utils::getFullPtsString (mPts)));
          }
        else
          cLog::log (LOGINFO, fmt::format ("skip missed:{}", utils::getFullPtsString (mPts)));

        if (audioFrame) {
          if (!mMute) {
            float* src = audioFrame->mSamples.data();
            float* dst = mSamples.data();
            switch (audioFrame->getNumChannels()) {
              //{{{
              case 1: // mono to 2 interleaved channels
                for (size_t i = 0; i < audioFrame->getSamplesPerFrame(); i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                break;
              //}}}
              //{{{
              case 2: // interleaved stereo to 2 interleaved channels
                memcpy (dst, src, audioFrame->getSamplesPerFrame() * 8);
                break;
              //}}}
              //{{{
              case 6: // interleaved 5.1 to 2 interleaved channels
                for (size_t i = 0; i < audioFrame->getSamplesPerFrame(); i++) {
                  *dst++ = src[0] + src[2] + src[3] + src[4]; // left
                  *dst++ = src[1] + src[2] + src[3] + src[5]; // right
                  src += 6;
                  }
                break;
              //}}}
              //{{{
              default:
                cLog::log (LOGERROR, fmt::format ("cAudioPlayer unknown num channels {}", audioFrame->getNumChannels()));
              //}}}
              }
            srcSamples = mSamples.data();
            }
          numSamples = (int)audioFrame->getSamplesPerFrame();
          ptsDuration = audioFrame->getPtsDuration();
          }
        }
      if (mPlaying)
        mPts += ptsDuration;

      #ifdef _WIN32
        }); // close lambda
      #else
        if (hasAudio)
          audio.play (2, srcSamples, numSamples, 1.f);
        else
          this_thread::sleep_for (21333us); // !!!! crude HD bodge for noAudio raspberry pi !!!
      #endif
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

cAudioPlayer::~cAudioPlayer() {
  mExit = true;
  while (mRunning)
    this_thread::sleep_for (1ms);
  }

// cSongPlayer.cpp - player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cSongPlayer.h"

#include <array>
#include <thread>

// utils
#include "../common/date.h"
#include "../common/utils.h"
#include "../common/cLog.h"

// song
#include "cSong.h"

#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

using namespace std;
//}}}

#ifdef _WIN32
  cSongPlayer::cSongPlayer (cSong* song, bool streaming) {
    thread playerThread = thread ([=]() {
      // player lambda
      cLog::setThreadName ("play");
      SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
      array <float,2048*2> silence = { 0.f };
      array <float,2048*2> samples = { 0.f };

      song->togglePlaying();
      //{{{  WSAPI player thread, video follows playPts
      auto device = getDefaultAudioOutputDevice();
      if (device) {
        cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", song->getSampleRate());
        device->setSampleRate (song->getSampleRate());
        device->start();

        cSong::cFrame* frame;
        while (!mExit) {
          device->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
            // loadSrcSamples callback lambda
            shared_lock<shared_mutex> lock (song->getSharedMutex());

            frame = song->findPlayFrame();
            if (song->getPlaying() && frame && frame->getSamples()) {
              if (song->getNumChannels() == 1) {
                // mono to stereo
                float* src = frame->getSamples();
                float* dst = samples.data();
                for (uint32_t i = 0; i < song->getSamplesPerFrame(); i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                }
              else
                memcpy (samples.data(), frame->getSamples(), song->getSamplesPerFrame() * song->getNumChannels() * sizeof(float));
              srcSamples = samples.data();
              }
            else
              srcSamples = silence.data();
            numSrcSamples = song->getSamplesPerFrame();

            if (frame && song->getPlaying())
              song->nextPlayFrame (true);
            });

          if (!streaming && song->getPlayFinished())
            break;
          }

        device->stop();
        }
      //}}}
      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });
    playerThread.detach();
    }

#else
  cSongPlayer::cSongPlayer (cSong* song, bool streaming) {
    thread playerThread = thread ([=]() { // ,this
      // player lambda
      cLog::setThreadName ("play");
      array <float,2048*2> silence = { 0.f };
      array <float,2048*2> samples = { 0.f };

      song->togglePlaying();
      //{{{  audio16 player thread, video follows playPts
      cAudio audio (2, song->getSampleRate(), 40000, false);

      cSong::cFrame* frame;
      while (!mExit) {
        float* playSamples = silence.data();
          {
          // scoped song mutex
          shared_lock<shared_mutex> lock (song->getSharedMutex());
          frame = song->findPlayFrame();
          bool gotSamples = song->getPlaying() && frame && frame->getSamples();
          if (gotSamples) {
            memcpy (samples.data(), frame->getSamples(), song->getSamplesPerFrame() * 8);
            playSamples = samples.data();
            }
          }
        audio.play (2, playSamples, song->getSamplesPerFrame(), 1.f);

        if (frame && song->getPlaying())
          song->nextPlayFrame (true);

        if (!streaming && song->getPlayFinished())
          break;
        }
      //}}}
      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });

    // raise to max prioritu
    sched_param sch_params;
    sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
    pthread_setschedparam (playerThread.native_handle(), SCHED_RR, &sch_params);
    playerThread.detach();
    }
#endif

void cSongPlayer::wait() {
  while (mRunning)
    this_thread::sleep_for (100ms);
  }

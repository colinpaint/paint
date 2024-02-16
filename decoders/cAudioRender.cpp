// cAudioRender.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

#include "cAudioRender.h"

#include <cstdint>
#include <string>
#include <array>
#include <algorithm>
#include <functional>
#include <thread>

#include "../common/cLog.h"
#include "../common/utils.h"

//{{{  include libav
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
#include "cAudioParser.h"
#include "cFFmpegAudioDecoder.h"
#include "cAudioPlayer.h"

using namespace std;
//}}}

//{{{
cAudioRender::cAudioRender (bool queue, size_t maxFrames, bool playerHasAudio,
                            uint8_t streamType, uint16_t pid)
    : cRender(queue, "aud", streamType, pid, 1920, maxFrames,
        // getFrame lambda
        [&](bool allocFront) noexcept {
          return hasMaxFrames() ? removeFirstFrame() : new cAudioFrame();
          },

        // addFrame lambda
        [&](cFrame* frame) noexcept {
          cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);
          if (mSampleRate != audioFrame->getSampleRate()) {
            cLog::log (LOGERROR, fmt::format ("cAudioRender::addFrame sampleRate changed {} to {}",
                                              mSampleRate, audioFrame->getSampleRate()));
            mSampleRate = audioFrame->getSampleRate();
            }

          setPts (frame->getPts(), frame->getPtsDuration());
          mSamplesPerFrame = audioFrame->getSamplesPerFrame();
          mFrameInfo = audioFrame->getInfoString();
          audioFrame->calcPower();

          cRender::addFrame (frame);

          if (!mAudioPlayer) {
            mAudioPlayer = new cAudioPlayer (*this, mSampleRate, getPid(), mPlayerHasAudio);
            mAudioPlayer->startPts (audioFrame->getPts());
            }
          }
        ),
      mSampleRate(48000), mSamplesPerFrame(1024), mPlayerHasAudio(playerHasAudio) {

  mDecoder = new cFFmpegAudioDecoder ((streamType == 17) ? eAudioFrameType::eAacLatm : eAudioFrameType::eMp3);
  }
//}}}

//{{{
cAudioFrame* cAudioRender::getAudioFrameAtPts (int64_t pts) {

  return (mFramesMap.empty() || !getPtsDuration()) ?
    nullptr : dynamic_cast<cAudioFrame*>(getFrameAtPts (pts));
  }
//}}}
//{{{
cAudioFrame* cAudioRender::getAudioFrameAtOrAfterPts (int64_t pts) {

  return (mFramesMap.empty() || !getPtsDuration()) ?
    nullptr : dynamic_cast<cAudioFrame*>(getFrameAtOrAfterPts (pts));
  }
//}}}

// overrides
//{{{
string cAudioRender::getInfoString() const {
  return fmt::format ("aud {} {} {}", cRender::getInfoString(), mDecoder->getInfoString(), mFrameInfo);
  }
//}}}

//{{{
bool cAudioRender::throttle() {

  if (mAudioPlayer)
    return cRender::throttle (mAudioPlayer->getPts());

  return false;
  }
//}}}

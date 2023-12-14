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

#include "../common/date.h"
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
#include "cFFmpegAudioDecoder.h"
#include "cPlayer.h"

using namespace std;
//}}}
constexpr bool kQueued = true;
constexpr int64_t kDefaultPtsPerFrame = 1920;
constexpr size_t kLiveMaxFrames = 24;
constexpr size_t kFileMaxFrames = 48;

// cAudioRender
//{{{
cAudioRender::cAudioRender (const string& name, uint8_t streamType, uint16_t pid, bool live, bool hasAudio)
    : cRender(kQueued, name, "aud ", streamType, pid,
              kDefaultPtsPerFrame, live, live ? kLiveMaxFrames : kFileMaxFrames,
              [&]() noexcept { return getFrame(); },
              [&](cFrame* frame) noexcept { addFrame (frame); }),
      mSampleRate(48000), mSamplesPerFrame(1024), mHasAudio(hasAudio) {

  mDecoder = new cFFmpegAudioDecoder (*this, streamType);
  }
//}}}

//{{{
cAudioFrame* cAudioRender::getAudioFrameAtPts (int64_t pts) {

  return (mFramesMap.empty() || !mPtsDuration) ?
    nullptr : dynamic_cast<cAudioFrame*>(getFrameAtPts (pts));
  }
//}}}
//{{{
cAudioFrame* cAudioRender::getAudioFrameAtOrAfterPts (int64_t pts) {

  if (mFramesMap.empty() || !mPtsDuration)
    return nullptr;
  else
    return dynamic_cast<cAudioFrame*>(getFrameAtOrAfterPts (pts));
  }
//}}}

// decoder callbacks
//{{{
cFrame* cAudioRender::getFrame() {
  return hasMaxFrames() ? reuseBestFrame() : new cAudioFrame();
  }
//}}}
//{{{
void cAudioRender::addFrame (cFrame* frame) {

  cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);

  if (mSampleRate != audioFrame->getSampleRate()) {
    cLog::log (LOGERROR, fmt::format ("cAudioRender::addFrame sampleRate changed from {} to {}",
                                      mSampleRate, audioFrame->getSampleRate()));
    mSampleRate = audioFrame->getSampleRate();
    }

  // save last pts and duration
  mPts = frame->getPts();
  mPtsDuration = frame->getPtsDuration();

  mSamplesPerFrame = audioFrame->getSamplesPerFrame();
  mFrameInfo = audioFrame->getInfoString();
  audioFrame->calcPower();

  cRender::addFrame (frame);

  // create and start player
  if (!mPlayer) {
    mPlayer = new cPlayer (*this, mSampleRate, getPid(), mHasAudio);
    mPlayer->startPlayPts (audioFrame->getPts());
    }
  }
//}}}

// overrides
//{{{
string cAudioRender::getInfoString() const {
  return fmt::format ("aud {} {} {}", cRender::getInfoString(), mDecoder->getInfoString(), mFrameInfo);
  }
//}}}
//{{{
bool cAudioRender::processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {

  if (mPlayer)
    while (throttle (mPlayer->getPts()))
      this_thread::sleep_for (1ms);

  return cRender::processPes (pid, pes, pesSize, pts, dts, skip);
  }
//}}}

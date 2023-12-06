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
constexpr bool kAudioQueued = true;
constexpr size_t kAudioFrameMapSize = 16;
constexpr int64_t kDefaultPtsPerAudioFrame = 1920;

// cAudioRender
//{{{
cAudioRender::cAudioRender (const string& name, uint8_t streamType, bool realTime)
    : cRender(kAudioQueued, name + "aud", streamType, kAudioFrameMapSize, kDefaultPtsPerAudioFrame, realTime),
      mSampleRate(48000), mSamplesPerFrame(1024) {

  mDecoder = new cFFmpegAudioDecoder (*this, streamType);

  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });

  mPlayer = new cPlayer (*this, mSampleRate);
  }
//}}}
//{{{
cAudioRender::~cAudioRender() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFramesMap)
    delete (frame.second);
  mFramesMap.clear();

  for (auto& frame : mFreeFrames)
    delete frame;

  mFreeFrames.clear();
  }
//}}}

//{{{
cAudioFrame* cAudioRender::getAudioFrameFromPts (int64_t pts) {

  if (mFramesMap.empty() || !mPtsDuration)
    return nullptr;
  else
    return dynamic_cast<cAudioFrame*>(getFrameFromPts (pts / mPtsDuration));
  }
//}}}
//{{{
cAudioFrame* cAudioRender::getAudioNearestFrameFromPts (int64_t pts) {

  if (mFramesMap.empty() || !mPtsDuration)
    return nullptr;
  else
    return dynamic_cast<cAudioFrame*>(getNearestFrameFromPts (pts / mPtsDuration));
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

  mPts = frame->mPts;
  mPtsDuration = frame->mPtsDuration;

  mSamplesPerFrame = audioFrame->mSamplesPerFrame;
  mFrameInfo = audioFrame->getInfoString();
  audioFrame->calcPower();

  { // locked emplace
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFramesMap.emplace (audioFrame->mPts / mPtsDuration, audioFrame);
  }

  // start player
  if (!getPlayer().isPlaying())
    getPlayer().startPlayPts (audioFrame->mPts);
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

  // throttle fileRead thread
  if (!getRealTime())
    while (mFramesMap.size() > mFrameMapSize)
      this_thread::sleep_for (1ms);

  return cRender::processPes (pid, pes, pesSize, pts, dts, skip);
  }
//}}}
//{{{
void cAudioRender::audioTrimBeforePts (int64_t pts) {
  trimBeforePts (pts / mPtsDuration);
  }
//}}}

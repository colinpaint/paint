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

#pragma warning (push)
#pragma warning (disable: 4244)
extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }
#pragma warning (pop)

#include "cFFmpegAudioDecoder.h"
#include "cPlayer.h"

using namespace std;
//}}}
constexpr bool kAudioQueued = true;
constexpr size_t kAudioFrameMapSize = 24;

// cAudioRender
//{{{
cAudioRender::cAudioRender (const string& name, uint8_t streamType, bool realTime)
    : cRender(kAudioQueued, name + "aud", streamType, kAudioFrameMapSize, realTime),
      mSampleRate(48000), mSamplesPerFrame(1024), mPtsDuration(0) {

  mDecoder = new cFFmpegAudioDecoder (*this, streamType);

  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });

  mPlayer = new cPlayer (*this, mSampleRate);
  }
//}}}
//{{{
cAudioRender::~cAudioRender() {

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
  if (!getPlayer().isPlaying())
    getPlayer().startPlayPts (audioFrame->mPts);
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

  trimFramesBeforePts (getPlayer().getPts() - mPtsDuration);

  if (!getRealTime())
    // throttle on number of decoded audioFrames
    while (mFrames.size() > mFrameMapSize) {
      this_thread::sleep_for (1ms);
      trimFramesBeforePts (getPlayer().getPts() - mPtsDuration);
      }

  return cRender::processPes (pid, pes, pesSize, pts, dts, skip);
  }
//}}}

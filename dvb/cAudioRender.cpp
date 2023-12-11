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
constexpr size_t kAudioFrameMapSize = 48;
constexpr size_t kAudioThrottle = kAudioFrameMapSize - 6;
constexpr int64_t kDefaultPtsPerAudioFrame = 1920;

// cAudioRender
//{{{
cAudioRender::cAudioRender (const string& name, uint8_t streamType, uint16_t pid, bool live)
    : cRender(kAudioQueued, name + "aud", streamType, pid,
              kAudioFrameMapSize, kDefaultPtsPerAudioFrame, live),
      mSampleRate(48000), mSamplesPerFrame(1024) {

  mDecoder = new cFFmpegAudioDecoder (*this, streamType);

  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });
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
cAudioFrame* cAudioRender::getAudioFrameAtPts (int64_t pts) {

  if (mFramesMap.empty() || !mPtsDuration)
    return nullptr;
  else
    return dynamic_cast<cAudioFrame*>(getFrameAtPts (pts));
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

  cFrame* frame = allocFreeFrame();
  if (frame)
    // use freeFrame
    return frame;
  else if (mFramesMap.size() < kAudioFrameMapSize)
    // allocate newFrame
    return new cAudioFrame();
  else {
    // reuse youngestFrame
    cLog::log (LOGINFO, fmt::format ("cAudioRender::getFrame youngestFrame"));
    return allocYoungestFrame();
    }
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

  mPts = frame->getPts();
  mPtsDuration = frame->getPtsDuration();

  mSamplesPerFrame = audioFrame->getSamplesPerFrame();
  mFrameInfo = audioFrame->getInfoString();
  audioFrame->calcPower();

  { // locked emplace
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFramesMap.emplace (audioFrame->getPts() / mPtsDuration, audioFrame);
  }

  // create and start player
  if (!mPlayer) {
    mPlayer = new cPlayer (*this, mSampleRate, mPid);
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

  // throttle fileRead thread if all frames used
  if (!getLive())
    while (mFramesMap.size() >= kAudioThrottle)
      this_thread::sleep_for (1ms);

  return cRender::processPes (pid, pes, pesSize, pts, dts, skip);
  }
//}}}

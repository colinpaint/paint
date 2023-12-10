// cVideoRender.cpp
//{{{  includes
#include "cVideoRender.h"
#include "cVideoFrame.h"

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <functional>

#include "../common/cDvbUtils.h"
#include "../app/cGraphics.h"

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/utils.h"

//{{{  include libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/motion_vector.h>
  #include <libavutil/frame.h>
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}
#include "cFFmpegVideoFrame.h"
#include "cFFmpegVideoDecoder.h"

using namespace std;
//}}}
constexpr bool kVideoQueued = true;
constexpr size_t kVideoFrameMapSize = 50;

// cVideoRender
//{{{
cVideoRender::cVideoRender (const string& name, uint8_t streamType, uint16_t pid, bool realTime)
    : cRender(kVideoQueued, name + "vid", streamType, pid, kVideoFrameMapSize, 90000/25, realTime) {

  mDecoder = new cFFmpegVideoDecoder (*this, streamType);
  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });
  }
//}}}
//{{{
cVideoRender::~cVideoRender() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFramesMap)
    delete frame.second;
  mFramesMap.clear();

  for (auto& frame : mFreeFrames)
    delete frame;

  mFreeFrames.clear();
  }
//}}}

//{{{
cVideoFrame* cVideoRender::getVideoFrameAtPts (int64_t pts) {

  if (mFramesMap.empty() || !mPtsDuration)
    return nullptr;
  else
    return dynamic_cast<cVideoFrame*>(getFrameAtPts (pts));
  }
//}}}
//{{{
cVideoFrame* cVideoRender::getVideoFrameAtOrAfterPts (int64_t pts) {

  if (mFramesMap.empty() || !mPtsDuration)
    return nullptr;
  else
    return dynamic_cast<cVideoFrame*>(getFrameAtOrAfterPts (pts));
  }
//}}}

// decoder callbacks
//{{{
cFrame* cVideoRender::getFrame() {

  cFrame* frame = allocFreeFrame();
  if (frame)
    // use freeFrame
    return frame;
  else if (mFramesMap.size() < kVideoFrameMapSize)
    // allocate newFrame
    return new cFFmpegVideoFrame();
  else {
    // reuse youngestFrame
    cLog::log (LOGINFO, fmt::format ("cVideoRender::getFrame - using youngestFrame"));
    return allocYoungestFrame();
    }
  }
//}}}
//{{{
void cVideoRender::addFrame (cFrame* frame) {

  mPts = frame->getPts();
  mPtsDuration = frame->getPtsDuration();

  cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);

  videoFrame->mQueueSize = getQueueSize();
  videoFrame->mTextureDirty = true;

  // save some videoFrame info
  mWidth = videoFrame->getWidth();
  mHeight = videoFrame->getHeight();
  mFrameInfo = videoFrame->getInfoString();

  { // locked
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFramesMap.emplace (videoFrame->getPts() / videoFrame->getPtsDuration(), videoFrame);
  }

  }
//}}}

// overrides
//{{{
string cVideoRender::getInfoString() const {
  return fmt::format ("vid {} {} {}", cRender::getInfoString(), mDecoder->getInfoString(), mFrameInfo);
  }
//}}}

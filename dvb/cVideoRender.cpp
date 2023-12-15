// cVideoRender.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

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
constexpr bool kQueued = true;
constexpr size_t kLiveMaxFrames = 30;
constexpr size_t kFileMaxFrames = 50;

// cVideoRender
//{{{
cVideoRender::cVideoRender (const string& name, uint8_t streamType, uint16_t pid, bool live)
    : cRender(kQueued, name, "vid ", streamType, pid,
              kPtsPer25HzFrame, live ? kLiveMaxFrames : kFileMaxFrames,

              // getFrame lambda
              [&]() noexcept {
                return hasMaxFrames() ? reuseBestFrame() : new cFFmpegVideoFrame();
                },

              // addFrame lambda
              [&](cFrame* frame) noexcept {
                mPts = frame->getPts();
                mPtsDuration = frame->getPtsDuration();
                cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
                videoFrame->mQueueSize = getQueueSize();
                videoFrame->mTextureDirty = true;

                // save some videoFrame info
                mWidth = videoFrame->getWidth();
                mHeight = videoFrame->getHeight();
                mFrameInfo = videoFrame->getInfoString();

                cRender::addFrame (frame);
                }
              ) {

  mDecoder = new cFFmpegVideoDecoder (*this, streamType);
  }
//}}}

//{{{
cVideoFrame* cVideoRender::getVideoFrameAtPts (int64_t pts) {

  return (mFramesMap.empty() || !mPtsDuration) ?
    nullptr : dynamic_cast<cVideoFrame*>(getFrameAtPts (pts));
  }
//}}}
//{{{
cVideoFrame* cVideoRender::getVideoFrameAtOrAfterPts (int64_t pts) {

  return (mFramesMap.empty() || !mPtsDuration) ?
    nullptr : dynamic_cast<cVideoFrame*>(getFrameAtOrAfterPts (pts));
  }
//}}}

// overrides
//{{{
string cVideoRender::getInfoString() const {
  return fmt::format ("vid {} {} {}", cRender::getInfoString(), mDecoder->getInfoString(), mFrameInfo);
  }
//}}}
//{{{
bool cVideoRender::processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {

  return cRender::processPes (pid, pes, pesSize, pts, dts, skip);
  }
//}}}

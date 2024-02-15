// cVideoRender.cpp
constexpr bool kAddFrameDebug = false;
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

#include "cVideoRender.h"

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
#include "../decoders/cFFmpegVideoFrame.h"
#include "../decoders/cFFmpegVideoDecoder.h"

using namespace std;
//}}}

// cVideoRender
//{{{
cVideoRender::cVideoRender (bool queue, size_t maxFrames,
                            const string& name, uint8_t streamType, uint16_t pid) :
    cRender(queue, name, "vid", streamType, pid, kPtsPer25HzFrame, maxFrames,
            // getFrame lambda
            [&]() noexcept {
              return hasMaxFrames() ? reuseBestFrame() : new cFFmpegVideoFrame();
              },
            // addFrame lambda
            [&](cFrame* frame) noexcept {
              if (kAddFrameDebug)
                cLog::log (LOGINFO, fmt::format ("- cVideoRender::addFrame {}", utils::getFullPtsString (frame->getPts())));

              setPts (frame->getPts(), frame->getPtsDuration());
              cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
              videoFrame->mQueueSize = getQueueSize();
              videoFrame->mTextureDirty = true;

              // save some videoFrame info
              mWidth = videoFrame->getWidth();
              mHeight = videoFrame->getHeight();
              mFrameInfo = videoFrame->getInfoString();

              cRender::addFrame (frame);
              }) {

  mDecoder = new cFFmpegVideoDecoder (streamType == 27);
  }
//}}}

//{{{
cVideoFrame* cVideoRender::getVideoFrameAtPts (int64_t pts) {

  return (mFramesMap.empty() || !getPtsDuration()) ?
    nullptr : dynamic_cast<cVideoFrame*>(getFrameAtPts (pts));
  }
//}}}
//{{{
cVideoFrame* cVideoRender::getVideoFrameAtOrAfterPts (int64_t pts) {

  return (mFramesMap.empty() || !getPtsDuration()) ?
    nullptr : dynamic_cast<cVideoFrame*>(getFrameAtOrAfterPts (pts));
  }
//}}}

// overrides
//{{{
string cVideoRender::getInfoString() const {
  return fmt::format ("vid {} {} {}", cRender::getInfoString(), mDecoder->getInfoString(), mFrameInfo);
  }
//}}}

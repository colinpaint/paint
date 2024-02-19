// cSubtitleRender.cpp
//{{{  includes
#include "cSubtitleRender.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../decoders/cSubtitleImage.h"
#include "../decoders/cSubtitleFrame.h"
#include "../decoders/cSubtitleDecoder.h"

#include "../common/cLog.h"
#include "../common/utils.h"

using namespace std;
using namespace utils;
//}}}
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))

#define BGRA(r,g,b,a) static_cast<uint32_t>(((a << 24) ) | (b << 16) | (g <<  8) | r)
//}}}
constexpr bool kAllocFrameDebug = false;
constexpr bool kAddFrameDebug = false;

// public:
//{{{
cSubtitleRender::cSubtitleRender (bool queue, size_t maxFrames, size_t preLoadFrames,
                                  uint8_t streamType, uint16_t pid) : 
    cRender(queue, "sub", streamType, pid, kPtsPer25HzFrame, maxFrames, preLoadFrames,
      // allocFrame lambda
      [&](int64_t pts) noexcept {
        if (kAllocFrameDebug)
          cLog::log (LOGINFO, fmt::format ("cSubtitleRender::allocFrame {} {}", getFullPtsString(pts)));
        return hasMaxFrames() ? new cSubtitleFrame() : new cSubtitleFrame();
        },

      // addFrameCall lambda
      [&](cFrame* frame) noexcept {
        setPts (frame->getPts(), frame->getPtsDuration());
        //cRender::addframe (frame);
        if (kAddFrameDebug)
          cLog::log (LOGINFO, fmt::format ("cSubtitleRender::addFrame {}", getPtsString (frame->getPts())));
        }
      ) {
  mDecoder = new cSubtitleDecoder (*this);
  }
//}}}

//{{{
size_t cSubtitleRender::getNumLines() const {
  return dynamic_cast<cSubtitleDecoder*>(mDecoder)->getNumLines();
  }
//}}}
//{{{
size_t cSubtitleRender::getHighWatermark() const {
  return dynamic_cast<cSubtitleDecoder*>(mDecoder)->getHighWatermark();
  }
//}}}
//{{{
cSubtitleImage& cSubtitleRender::getImage (size_t line) {
  return dynamic_cast<cSubtitleDecoder*>(mDecoder)->getImage (line);
  }
//}}}

// overrides
//{{{
string cSubtitleRender::getInfoString() const {
  return mDecoder->getInfoString();
  }
//}}}

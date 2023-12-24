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

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/utils.h"

using namespace std;
//}}}
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))

#define BGRA(r,g,b,a) static_cast<uint32_t>(((a << 24) ) | (b << 16) | (g <<  8) | r)
//}}}
constexpr bool kQueued = true;
constexpr size_t kMaxFrames = 0;

// public:
//{{{
cSubtitleRender::cSubtitleRender (const string& name, uint8_t streamType, uint16_t pid, iOptions* options)
    : cRender(kQueued, name, "sub", options, streamType, pid, kPtsPer25HzFrame, kMaxFrames,

              // getFrame lambda
              [&]() noexcept {
                // !!! no relloacate yet !!!
                return hasMaxFrames() ? new cSubtitleFrame() : new cSubtitleFrame();
                },

              // addFrame lambda
              [&](cFrame* frame) noexcept {
                setPts (frame->getPts(), frame->getPtsDuration(), frame->getStreamPos());
                //cRender::addframe (frame);
                //cLog::log (LOGINFO, fmt::format ("subtitle addFrame {}", getPtsString (frame->mPts)));
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

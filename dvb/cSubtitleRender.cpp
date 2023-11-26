// cSubtitleRender.cpp
//{{{  includes
#include "cSubtitleRender.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/utils.h"

#include "cSubtitleFrame.h"
#include "cSubtitleDecoder.h"
#include "cSubtitleImage.h"

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
constexpr size_t kSubtitleMapSize = 0;

// public:
//{{{
cSubtitleRender::cSubtitleRender (const string& name, uint8_t streamType, bool realTime)
    : cRender(kQueued, name + "sub", streamType, kSubtitleMapSize, 90000/25, realTime) {

  mDecoder = new cSubtitleDecoder (*this);

  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });
  }
//}}}
cSubtitleRender::~cSubtitleRender() = default;

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

// decoder callbacks - ??? should mak ethem work for pts presentation ????
//{{{
cFrame* cSubtitleRender::getFrame() {
  return new cSubtitleFrame();
  }
//}}}
//{{{
void cSubtitleRender::addFrame (cFrame* frame) {

  mPts = frame->mPts;
  mPtsDuration = frame->mPtsDuration;
  //cLog::log (LOGINFO, fmt::format ("subtitle addFrame {}", getPtsString (frame->mPts)));
  }
//}}}

// overrides
//{{{
string cSubtitleRender::getInfoString() const {
  return mDecoder->getInfoString();
  }
//}}}

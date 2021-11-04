// cRender.cpp - dvb stream render base class
//{{{  includes
#include "cRender.h"

#include <cstdint>
#include <string>
#include <map>

#include "cDecoder.h"

#include "../utils/cLog.h"

using namespace std;
//}}}
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
//}}}
constexpr int64_t kPtsPerFrame = 90000 / 25;

// public:
cRender::cRender (const std::string name) : mName(name), mMiniLog ("log") {}
//{{{
cRender::~cRender() {
  delete mDecoder;
  }
//}}}

void cRender::toggleLog() { mMiniLog.toggleEnable(); }

//{{{
float cRender::getValue (int64_t pts) const {

  auto it = mValuesMap.find (pts / kPtsPerFrame);
  return it == mValuesMap.end() ? 0.f : it->second;
  }
//}}}
//{{{
float cRender::getOffsetValue (int64_t ptsOffset, int64_t& pts) const {

  pts = mRefPts - ptsOffset;
  auto it = mValuesMap.find (pts / kPtsPerFrame);
  return it == mValuesMap.end() ? 0.f : it->second;
  }
//}}}

//{{{
void cRender::logValue (int64_t pts, float value) {

  while (mValuesMap.size() >= mMapSize)
    mValuesMap.erase (mValuesMap.begin());

  mValuesMap.emplace (pts / kPtsPerFrame, value);

  if (pts > mLastPts)
    mLastPts = pts;
  }
//}}}

// protected:
void cRender::header() { mMiniLog.setHeader (fmt::format ("header")); }
void cRender::log (const string& tag, const string& text) { mMiniLog.log (tag, text); }

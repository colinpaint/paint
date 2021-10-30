// cDecoder.cpp - dvb stream decoder base class
//{{{  includes
#include "cDecoder.h"

#include <cstdint>
#include <string>
#include <map>

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
cDecoder::cDecoder (const std::string name) : mName(name), mMiniLog ("log") {}
cDecoder::~cDecoder() {}

void cDecoder::toggleLog() { mMiniLog.toggleEnable(); }

//{{{
float cDecoder::getValue (int64_t pts) const {

  auto it = mValuesMap.find (pts / kPtsPerFrame);
  return it == mValuesMap.end() ? 0.f : (it->second / mMaxValue);
  }
//}}}
//{{{
float cDecoder::getOffsetValue (int64_t ptsOffset) const {

  auto it = mValuesMap.find ((mRefPts - ptsOffset) / kPtsPerFrame);
  return it == mValuesMap.end() ? 0.f : (it->second / mMaxValue);
  }
//}}}

//{{{
void cDecoder::logValue (int64_t pts, float value) {

  if (mValuesMap.size() > mMapSize)
    mValuesMap.erase (mValuesMap.begin());

  mValuesMap.insert (map<int64_t,float>::value_type (pts / kPtsPerFrame, value));

  if (value > mMaxValue)
    mMaxValue = value;

  if (pts > mLastPts)
    mLastPts = pts;
  }
//}}}

// protected:
void cDecoder::header() { mMiniLog.setHeader (fmt::format ("header")); }
void cDecoder::log (const string& tag, const string& text) { mMiniLog.log (tag, text); }

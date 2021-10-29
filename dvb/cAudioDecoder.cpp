// cAudioDecoder.cpp
//{{{  includes
#include "cAudioDecoder.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

using namespace std;
//}}}
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
//}}}

// public:
cAudioDecoder::cAudioDecoder (const std::string name) : mName(name), mMiniLog ("subLog") {}
//{{{
cAudioDecoder::~cAudioDecoder() {
  }
//}}}

//{{{
void cAudioDecoder::toggleLog() {
  mMiniLog.toggleEnable();
  }
//}}}

//{{{
bool cAudioDecoder::decode (const uint8_t* buf, int bufSize, int64_t pts) {

  //mPage.mPts = pts;
  //mPage.mPesSize = bufSize;

  log ("pes", fmt::format ("pts:{} size: {}", getFullPtsString (pts), bufSize));

  const uint8_t* bufEnd = buf + bufSize;
  const uint8_t* bufPtr = buf + 2;

  while (bufEnd - bufPtr >= 6) {
    bufPtr++;
    }

  return false;
  }
//}}}

// private:
//{{{
void cAudioDecoder::header() {

  mMiniLog.setHeader (fmt::format ("header"));
  }
//}}}
//{{{
void cAudioDecoder::log (const string& tag, const string& text) {
  mMiniLog.log (tag, text);
  }
//}}}

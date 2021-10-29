// cVideoDecoder.cpp
//{{{  includes
#include "cVideoDecoder.h"

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
cVideoDecoder::cVideoDecoder (const std::string name) : cDecoder(name) {}
//{{{
cVideoDecoder::~cVideoDecoder() {
  }
//}}}

//{{{
bool cVideoDecoder::decode (const uint8_t* buf, int bufSize, int64_t pts) {

  (void)buf;
  log ("pes", fmt::format ("pts:{} size:{}", getFullPtsString (pts), bufSize));

  logValue (pts, (float)bufSize);
  return false;
  }
//}}}

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
void cVideoDecoder::decode (uint8_t* pes, uint32_t pesSize, int64_t pts) {

  (void)pes;
  log ("pes", fmt::format ("pts:{} size:{}", getFullPtsString (pts), pesSize));

  logValue (pts, (float)pesSize);
  }
//}}}

// cAudioDecoder.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cDecoder.h"
#include "../utils/cMiniLog.h"
//}}}

class cAudioDecoder : public cDecoder {
public:
  cAudioDecoder (const std::string name);
  virtual ~cAudioDecoder();

  bool decode (const uint8_t* buf, int bufSize, int64_t pts);
  };

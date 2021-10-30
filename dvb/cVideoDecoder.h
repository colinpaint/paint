// cVideoDecoder.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cDecoder.h"
#include "../utils/cMiniLog.h"
//}}}

class cVideoDecoder : public cDecoder {
public:
  cVideoDecoder (const std::string name);
  virtual ~cVideoDecoder();

  bool decode (uint8_t* buf, int bufSize, int64_t pts);
  };

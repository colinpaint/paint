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

  void decode (uint8_t* pes, uint32_t pesSize, int64_t pts);
  };

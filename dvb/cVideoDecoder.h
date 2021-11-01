// cVideoDecoder.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cDecoder.h"
#include "../utils/cMiniLog.h"

class cVideoPool;
//}}}

class cVideoDecoder : public cDecoder {
public:
  cVideoDecoder (const std::string name);
  virtual ~cVideoDecoder();

  int getWidth();
  int getHeight();
  uint32_t* getFrame (int64_t pts);

  void decode (uint8_t* pes, uint32_t pesSize, int64_t pts);

private:
  cVideoPool* mVideoPool;
  };

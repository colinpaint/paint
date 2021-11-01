// cVideoDecoder.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cDecoder.h"
#include "../utils/cMiniLog.h"

class cVideoPool;
class cTexture;
//}}}

class cVideoDecoder : public cDecoder {
public:
  cVideoDecoder (const std::string name);
  virtual ~cVideoDecoder();

  uint16_t getWidth() const;
  uint16_t getHeight() const ;
  uint32_t* getFramePixels (int64_t pts) const;

  void decode (uint8_t* pes, uint32_t pesSize, int64_t pts);

  cTexture* mTexture = nullptr;

private:
  cVideoPool* mVideoPool;
  };

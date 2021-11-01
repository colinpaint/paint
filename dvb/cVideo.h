// cVideo.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cVideoPool;
class cTexture;
//}}}

class cVideo : public cRender {
public:
  cVideo (const std::string name);
  virtual ~cVideo();

  uint16_t getWidth() const;
  uint16_t getHeight() const ;
  uint32_t* getFramePixels (int64_t pts) const;

  void process (uint8_t* pes, uint32_t pesSize, int64_t pts);

  cTexture* mTexture = nullptr;

private:
  cVideoPool* mVideoPool;
  };

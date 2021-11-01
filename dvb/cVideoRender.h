// cVideoRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cVideoPool;
class cTexture;
//}}}

class cVideoRender : public cRender {
public:
  cVideoRender (const std::string name);
  virtual ~cVideoRender();

  uint16_t getWidth() const;
  uint16_t getHeight() const ;
  uint8_t* getFramePixels (int64_t pts) const;

  void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts);

  int64_t mPts = 0;
  cTexture* mTexture = nullptr;

private:
  cVideoPool* mVideoPool;
  };

// cVideoRender.h
//{{{  includes
#pragma once
#include "cRender.h"

#include <cstdint>
#include <string>

#include "../common/basicTypes.h"
#include "../app/cGraphics.h"

class cTexture;
class cVideoFrame;
//}}}

class cVideoRender : public cRender {
public:
  cVideoRender (const std::string& name, uint8_t streamType, bool realTime);
  virtual ~cVideoRender();

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }

  cVideoFrame* getVideoFrameFromPts (int64_t pts);
  cVideoFrame* getVideoNearestFrameFromPts (int64_t pts);

  // callbacks
  cFrame* getFrame();
  void addFrame (cFrame* frame);

  // overrides
  virtual std::string getInfoString() const final;
  virtual void videoTrimBeforePts (int64_t pts) final;

private:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;

  std::string mFrameInfo;
  };

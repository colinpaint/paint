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
  //{{{
  class cOptions {
  public:
    virtual ~cOptions() = default;
    };
  //}}}

  cVideoRender (bool queue, size_t maxFrames,
                const std::string& name, uint8_t streamType, uint16_t pid, iOptions* options);
  virtual ~cVideoRender() = default;

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }

  cVideoFrame* getVideoFrameAtPts (int64_t pts);
  cVideoFrame* getVideoFrameAtOrAfterPts (int64_t pts);

  // overrides
  virtual std::string getInfoString() const final;

private:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;

  std::string mFrameInfo;
  };

// cVideoRender.h
//{{{  includes
#pragma once
#include "cRender.h"

#include <cstdint>
#include <string>

class cTexture;
class cGraphics;
class cVideoFrame;
//}}}

class cVideoRender : public cRender {
public:
  cVideoRender (const std::string& name, uint8_t streamTypeId, uint16_t decoderMask);
  virtual ~cVideoRender();

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }
  int64_t getPtsDuration() const { return mPtsDuration; }

  cVideoFrame* getPtsFrame (int64_t pts);
  cTexture* getTexture (int64_t pts, cGraphics& graphics);

  // virtuals
  virtual std::string getInfo() const final;
  virtual void addFrame (cFrame* frame) final;

private:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;

  int64_t mLastPts = -1;
  int64_t mPtsDuration = 0;

  std::string mInfo;
  };

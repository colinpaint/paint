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

constexpr uint32_t kVideoFrameMapSize = 30;

class cVideoRender : public cRender {
public:
  cVideoRender (const std::string& name, uint8_t streamType, uint16_t decoderMask);
  virtual ~cVideoRender();

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }
  int64_t getPtsDuration() const { return mPtsDuration; }

  cVideoFrame* getVideoFrameFromPts (int64_t pts);
  cVideoFrame* getVideoNearestFrameFromPts (int64_t pts);

  void trimVideoBeforePts (int64_t pts);

  // callbacks
  cFrame* getFrame();
  void addFrame (cFrame* frame);

  virtual std::string getInfoString() const final;

private:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  int64_t mPtsDuration = 90000 / 25;

  std::string mFrameInfo;
  };

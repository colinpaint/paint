// cVideoRender.h
//{{{  includes
#pragma once
#include "cRender.h"

#include <cstdint>
#include <string>

#include "../app/cGraphics.h"

class cTexture;
class cVideoFrame;
//}}}

class cVideoRender : public cRender {
public:
  cVideoRender (const std::string& name, uint8_t streamType);
  virtual ~cVideoRender();

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }
  int64_t getPtsDuration() const { return mPtsDuration; }

  cVideoFrame* getVideoFrameFromPts (int64_t pts);
  cVideoFrame* getVideoNearestFrameFromPts (int64_t pts);

  // callbacks
  cFrame* getFrame();
  void addFrame (cFrame* frame);

  void drawFrame (cVideoFrame* videoFrame, cGraphics& graphics, const cMat4x4& model, int width, int height);

  // overrides
  virtual std::string getInfoString() const final;
  virtual void trimVideoBeforePts (int64_t pts) final;

private:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  int64_t mPtsDuration = 90000 / 25;

  cQuad* mQuad = nullptr;

  std::string mFrameInfo;
  };

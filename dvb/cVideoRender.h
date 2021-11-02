// cVideoRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <shared_mutex>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cVideoFrame;
class cVideoDecoder;
class cGraphics;
class cTexture;
//}}}

class cVideoRender : public cRender {
public:
  cVideoRender (const std::string name);
  virtual ~cVideoRender();

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }
  std::string getInfoString() const;

  uint8_t* getFramePixels (int64_t pts);
  cTexture* getTexture (int64_t playPts, cGraphics& graphics);

  void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts);

private:
  void addFrame (cVideoFrame* frame);

  // vars
  cVideoDecoder* mVideoDecoder = nullptr;

  const size_t mMaxPoolSize;
  std::map <int64_t, cVideoFrame*> mFrames;

  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  int64_t mPtsDuration = 0;

  int64_t mGuessPts = -1;
  bool mSeenIFrame = false;

  int64_t mDecodeTime = 0;
  int64_t mYuv420Time = 0;

  int64_t mTexturePts = 0;
  cTexture* mTexture = nullptr;
  };

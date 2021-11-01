// cVideoRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <shared_mutex>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cVideoFrame;
class cTexture;
struct AVCodecParserContext;
struct AVCodec;
struct AVCodecContext;
struct SwsContext;
//}}}

class cVideoRender : public cRender {
public:
  cVideoRender (const std::string name);
  virtual ~cVideoRender();

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }
  uint8_t* getFramePixels (int64_t pts);
  std::string getInfoString() const;

  void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts);

  int64_t mPts = 0;
  cTexture* mTexture = nullptr;

private:
  cVideoFrame* findFrame (int64_t pts);
  cVideoFrame* findFreeFrame (int64_t pts);

  const bool mPlanar;
  const size_t mMaxPoolSize;

  std::shared_mutex mSharedMutex;
  std::map <int64_t, cVideoFrame*> mFrameMap;

  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  int64_t mPtsDuration = 0;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;
  SwsContext* mSwsContext = nullptr;

  int64_t mGuessPts = -1;
  bool mSeenIFrame = false;

  int64_t mDecodeTime = 0;
  int64_t mYuv420Time = 0;
  };

// cVideoFrame.h
//{{{  includes
#pragma once

#include "cRender.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include "../graphics/cGraphics.h"
//}}}

class cVideoFrame : public cFrame {
public:
  cVideoFrame (cTexture::eTextureType textureType,
               uint16_t width, uint16_t height, uint16_t stride, int64_t ptsDuration,
               int64_t pts, char frameType, uint32_t pesSize, int64_t decodeTime)
      : cFrame(pts, ptsDuration, pesSize),
        mWidth(width), mHeight(height), mStride(stride),
        mTextureType(textureType), mFrameType(frameType),
        mPesSize(pesSize) {

    mTimes.push_back (decodeTime);
    }

  virtual ~cVideoFrame() {
    delete mTexture;
    }

  // gets
  cTexture::eTextureType getTextureType() const { return mTextureType; }

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }
  uint16_t getStride() const { return mStride; }
  char getFrameType() const { return mFrameType; }

  uint32_t getPesSize() const { return mPesSize; }
  int64_t getDecodeTime() const { return mTimes[0]; }
  //{{{
  std::string getInfo() {

    std::string info = fmt::format ("{}x{} {}", mWidth, mHeight, mPesSize);

    for (auto& time : mTimes)
      info += fmt::format ("{:5} ", time);

    return info;
    }
  //}}}
  size_t getQueueSize() const { return mQueueSize; }

  virtual uint8_t** getPixelData() = 0;

  void addTime (int64_t time) { mTimes.push_back (time); }
  void setQueueSize (size_t queueSize) { mQueueSize = queueSize; }

  cTexture* mTexture = nullptr;

protected:
  const uint16_t mWidth;
  const uint16_t mHeight;
  const uint16_t mStride;

private:
  const cTexture::eTextureType mTextureType;
  const char mFrameType;
  const uint32_t mPesSize;
  std::vector <int64_t> mTimes;
  size_t mQueueSize = 0;
  };

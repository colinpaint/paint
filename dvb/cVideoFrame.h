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
  cVideoFrame() : cFrame() {}

  virtual ~cVideoFrame() {
    delete mTexture;
    }

  //{{{
  std::string getInfo() {

    std::string info = fmt::format ("{}x{} {}", mWidth, mHeight, mPesSize);

    for (auto& time : mTimes)
      info += fmt::format ("{:5} ", time);

    return info;
    }
  //}}}
  virtual uint8_t** getPixelData() = 0;
  virtual void releaseData() = 0;

  void addTime (int64_t time) { mTimes.push_back (time); }
  //{{{
  void updateTexture (cGraphics& graphics, const cPoint& size) {

    if (!mTexture) {
      mTexture = graphics.createTexture (mTextureType, size);
      mTextureDirty = true;
      }

    if (mTextureDirty) {
      mTexture->setPixels (getPixelData());
      releaseData();
      mTextureDirty = false;
      }
    }
  //}}}

  // vars
  cTexture::eTextureType mTextureType;

  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  uint16_t mStride = 0;

  char mFrameType = '?';

  size_t mQueueSize = 0;
  uint32_t mPesSize = 0;
  std::vector <int64_t> mTimes;

  bool mTextureDirty = true;
  cTexture* mTexture = nullptr;
  };

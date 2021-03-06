// cVideoFrame.h
//{{{  includes
#pragma once

#include "cRender.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include "../app/cGraphics.h"
//}}}

class cVideoFrame : public cFrame {
public:
  cVideoFrame(cTexture::eTextureType textureType) : mTextureType(textureType) {}
  //{{{
  virtual ~cVideoFrame() { 
    releaseResources();
    delete mTexture; 
    }
  //}}}

  //{{{
  std::string getInfo() {

    std::string info = fmt::format ("{}x{} {:5} ", mWidth, mHeight, mPesSize);
    for (auto time : mTimes)
      info += fmt::format ("{}:", time);

    return info;
    }
  //}}}
  //{{{
  virtual void releaseResources() final {

    mQueueSize = 0;
    mTimes.clear();
    mTextureDirty = true;
    }
  //}}}

  void addTime (int64_t time) { mTimes.push_back (time); }
  //{{{
  cTexture& getTexture (cGraphics& graphics) {
  // create and access texture for frame, release any intermeidate pixel data

    if (!mTexture) {
      mTexture = graphics.createTexture (mTextureType, cPoint (mWidth, mHeight));
      mTextureDirty = true;
      }

    if (mTextureDirty) {
      mTexture->setPixels (getPixels());
      mTextureDirty = false;
      }

    releasePixels();
    return *mTexture;
    }
  //}}}

  // vars
  const cTexture::eTextureType mTextureType;

  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  uint16_t mStride = 0;
  char mFrameType = '?';

  size_t mQueueSize = 0;
  std::vector <int64_t> mTimes;
  bool mTextureDirty = true;


protected:
  virtual uint8_t** getPixels() = 0;
  virtual void releasePixels() = 0;

  cTexture* mTexture = nullptr;
  };

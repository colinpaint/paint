// cVideoFrame.h
//{{{  includes
#pragma once

#include "../decoders/cFrame.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include "../common/basicTypes.h"
#include "../app/cGraphics.h"
//}}}

class cVideoFrame : public cFrame {
public:
  cVideoFrame(cTexture::eTextureType textureType) : mTextureType(textureType) {}
  //{{{
  virtual ~cVideoFrame() {

    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cVideoFrame::~cVideoFrame"));

    releaseResources();
    delete mTexture;
    }
  //}}}

  uint16_t getWidth() const { return mWidth; }
  uint16_t getHeight() const { return mHeight; }
  cPoint getSize() const { return { mWidth, mHeight }; }
  char getFrameType() { return mFrameType; }
  //{{{
  std::string getInfoString() {

  std::string info = fmt::format ("{}x{}:{}{} pesSize:{:5}",
                                  mWidth, mHeight,
                                  mInterlaced ? "I" : "P", mInterlaced ? (mTopFieldFirst ? "1" : "2") : "",
                                  getPesSize());
  if (!mTimes.empty()) {
    info += " took";
    for (auto time : mTimes)
      info += fmt::format (" {:5}us", time);
    }

  return info;
  }
  //}}}
  std::string getFrameInfo() const { return mFrameInfo; }

  virtual void* getMotionVectors (size_t& numMotionVectors) = 0;

  void setWidth (uint16_t width) { mWidth = width; }
  void setHeight (uint16_t height) { mHeight = height; }
  void setFrameType (char frameType) { mFrameType = frameType; }
  void setFrameInfo (const std::string& frameInfo) { mFrameInfo = frameInfo; }

  //{{{
  virtual void releaseResources() final {

    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cVideoFrame::releaseResources"));

    releasePixels();

    mQueueSize = 0;
    mTimes.clear();

    mTextureDirty = true;
    }
  //}}}

  //{{{
  void addTime (int64_t logTime) {
    mTimes.push_back (logTime);
    }
  //}}}
  //{{{
  cTexture& getTexture (cGraphics& graphics) {
  // create and access texture for frame, release any intermeidate pixel data

    if (!mTexture) {
      mTexture = graphics.createTexture (mTextureType, cPoint (mWidth, mHeight));
      mTextureDirty = true;
      }

    if (mTextureDirty) {
      mTexture->setPixels (getPixels(), getStrides());
      mTextureDirty = false;
      }

    releasePixels();
    return *mTexture;
    }
  //}}}

  // vars
  const cTexture::eTextureType mTextureType;

  uint16_t mStrideY = 0;
  uint16_t mStrideUV = 0;
  bool mInterlaced = false;
  bool mTopFieldFirst = false;

  char mFrameType = '?';

  bool mTextureDirty = true;

  size_t mQueueSize = 0;
  std::vector <int64_t> mTimes;

protected:
  virtual uint8_t** getPixels() = 0;
  virtual int* getStrides() = 0;
  virtual void releasePixels() {}

  // vars
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;

  std::string mFrameInfo;

  cTexture* mTexture = nullptr;
  };

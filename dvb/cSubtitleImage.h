// cSubtitleImage.h
//{{{  includes
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../common/basicTypes.h"
//}}}

class cSubtitleImage {
public:
  cSubtitleImage() {}
  ~cSubtitleImage() { free (mPixels); }

  int getXpos() const { return mX; }
  int getYpos() const { return mY; }

  int getWidth() const { return mWidth; }
  int getHeight() const { return mHeight; }
  cPoint getSize() const { return cPoint(mWidth, mHeight); }

  bool isDirty() const { return mDirty; }
  bool hasPixels() { return mPixels; }
  uint8_t** getPixels() { return &mPixels; }

  void setXpos (int x) { mX = x; }
  void setYpos (int y) { mY = y; }
  void setWidth (int width) { mWidth = width; }
  void setHeight (int height) { mHeight = height; }

  void setDirty (bool dirty) { mDirty = dirty; }

  //{{{
  void allocPixels() {
    mPixels = (uint8_t*)malloc (mWidth * mHeight * sizeof(uint32_t));
    }
  //}}}
  //{{{
  void releasePixels() {
    free (mPixels);
    mPixels = nullptr;
    }
  //}}}
  //{{{
  void setPixels (int width, int height, uint8_t* pixBuf) {

    // lookup region->mPixBuf through lut to produce mPixels
    uint32_t* ptr = (uint32_t*)mPixels;
    for (size_t i = 0; i < width * height; i++)
      *ptr++ = mColorLut[pixBuf[i]];

    mDirty = true;
    }
  //}}}

  uint8_t mPageState = 0;
  uint8_t mPageVersion = 0;

  std::array <uint32_t,16> mColorLut = { 0 };

private:
  int mX = 0;
  int mY = 0;

  int mWidth = 0;
  int mHeight = 0;

  bool mDirty = false;
  uint8_t* mPixels = nullptr;
  };

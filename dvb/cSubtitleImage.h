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

  int getWidth() const { return mWidth; }
  int getHeight() const { return mHeight; }
  cPoint getSize() const { return cPoint(mWidth, mHeight); }

  int mX = 0;
  int mY = 0;
  int mWidth = 0;
  int mHeight = 0;

  bool mDirty = false;
  uint8_t* mPixels = nullptr;
  std::array <uint32_t,16> mColorLut = { 0 };

  uint8_t mPageState = 0;
  uint8_t mPageVersion = 0;
  };

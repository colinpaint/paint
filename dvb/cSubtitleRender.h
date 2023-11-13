// cSubtitleRender.h
//{{{  includes
#pragma once
#include "cRender.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>

#include "../common/cMiniLog.h"

class cTexture;
//}}}

#define BGRA(r,g,b,a) static_cast<uint32_t>(((a << 24) ) | (b << 16) | (g <<  8) | r)
//{{{
class cSubtitleImage {
public:
  cSubtitleImage() {}
  ~cSubtitleImage() { free (mPixels); }

  uint8_t mPageState = 0;
  uint8_t mPageVersion = 0;

  int mX = 0;
  int mY = 0;
  int mWidth = 0;
  int mHeight = 0;

  bool mDirty = false;
  std::array <uint32_t,16> mColorLut = {0};
  uint8_t* mPixels = nullptr;

  cTexture* mTexture = nullptr;
  };
//}}}

class cSubtitleRender : public cRender {
public:
  cSubtitleRender (const std::string& name, uint8_t streamType);
  ~cSubtitleRender();

  size_t getNumLines() const;
  size_t getHighWatermark() const;
  cSubtitleImage& getImage (size_t line);

  // callbacks
  cFrame* getFrame();
  void addFrame (cFrame* frame);

  // overrides
  virtual std::string getInfoString() const final;
  };

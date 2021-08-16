// cLayer.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// glm
#include <mat4x4.hpp>

#include "../graphics/cGraphics.h"

class cBrush;
//}}}

class cLayer {
public:
  cLayer (cPoint size, cFrameBuffer::eFormat format, cGraphics& graphics);
  cLayer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format, cGraphics& graphics);
  ~cLayer();

  // gets
  cPoint getSize() { return mSize; }

  bool isVisible() { return mVisible; }
  std::string getName() { return mName; }

  uint32_t getTextureId();
  uint8_t* getPixels();
  uint8_t* getRenderedPixels();

  float getHue() { return mHue; }
  float getSat() { return mSat; }
  float getVal() { return mVal; }

  // sets
  void setName (const std::string& name) { mName = name; }
  void setVisible (bool visible) { mVisible = visible; }
  void setHueSatVal (float hue, float sat, float val) { mHue = hue; mSat = sat; mVal = val; }

  // actions
  void paint (cBrush* brush, cVec2 pos, bool first);
  void draw (const cPoint& size);

  void replace (uint8_t* pixels);
  void update (const cRect& rect);

private:
  // vars
  const cPoint mSize;
  const cFrameBuffer::eFormat mFormat;
  std::string mName;
  cGraphics& mGraphics;

  cQuad* mQuad = nullptr;

  bool mVisible = true;
  float mHue = 0.f;
  float mSat = 0.f;
  float mVal = 0.f;

  cFrameBuffer* mFrameBuffer = nullptr;
  cFrameBuffer* mFrameBuffer1 = nullptr;
  };

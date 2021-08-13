// cFrameBuffer.h  - Frame Buffer Object wrapper
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// glm
#include <vec4.hpp>

#include "cPointRect.h"
//}}}

class cFrameBuffer {
public:
  enum eFormat { eRGB, eRGBA };

  cFrameBuffer();
  cFrameBuffer (cPoint size, eFormat format);
  cFrameBuffer (uint8_t* pixels, cPoint size, eFormat format);
  ~cFrameBuffer();

  /// gets
  cPoint getSize() { return mSize; }
  unsigned getId() { return mFrameBufferObject; }
  unsigned getTextureId() { return mColorTextureId; }
  uint8_t* getPixels();
  unsigned getNumPixels() { return mSize.x * mSize.y; }
  unsigned getNumPixelBytes() { return  mSize.x * mSize.y * 4; }

  //sets
  void setSize (cPoint size);
  void setTarget (const cRect& rect);
  void setTarget() { setTarget (cRect (mSize)); }
  void setBlend();
  void setSource();

  // actions
  void invalidate();
  void pixelsChanged (const cRect& rect);

  void clear (const glm::vec4& color);
  void blit (cFrameBuffer* src, cPoint srcPoint, const cRect& dstRect);

  bool checkStatus();
  void reportInfo();

private:
  cPoint mSize = { 0,0 };
  const int mImageFormat;
  const int mInternalFormat;

  uint32_t mFrameBufferObject = 0;
  uint32_t mColorTextureId = 0;
  uint8_t* mPixels = nullptr;
  cRect mDirtyPixelsRect = cRect(0,0,0,0);
  };

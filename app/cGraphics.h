// cGraphics.h - static register and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

#include "../common/basicTypes.h"

struct ImDrawData;
//}}}

// abstract base classes
//{{{
class cShader {
// abstract bas class for all shaders
public:
  cShader() = default;
  virtual ~cShader() = default;

  uint32_t getId() { return mId; }
  virtual void use() = 0;

protected:
  uint32_t mId = 0;
  };
//}}}
//{{{
class cQuadShader : public cShader {
// abstract base class for shaders drawing quad triangle pair, with model, projection maths

public:
  cQuadShader() : cShader() {}
  virtual ~cQuadShader() = default;

  // sets
  virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) = 0;
  };
//}}}

//{{{
class cTexture {
public:
  enum eTextureType { eRgba, eYuv420 };

  cTexture (eTextureType textureType, const cPoint& size) : mTextureType(textureType), mSize(size) {}
  virtual ~cTexture() = default;

  /// gets
  eTextureType getTextureType() const { return mTextureType; }

  int32_t getWidth() const { return mSize.x; }
  int32_t getHeight() const { return mSize.y; }
  const cPoint getSize() const { return mSize; }

  virtual void* getTextureId() = 0;

  virtual void setPixels (uint8_t** pixels, int* strides) = 0;
  virtual void setSource() = 0;

protected:
  const eTextureType mTextureType;
  cPoint mSize;
  };
//}}}
//{{{
class cTextureShader : public cQuadShader {
public:
  cTextureShader() : cQuadShader() {}
  virtual ~cTextureShader() = default;
  };
//}}}

//{{{
class cQuad {
public:
  cQuad (const cPoint& size) : mSize(size) {}
  virtual ~cQuad() = default;

  cPoint getSize()  { return mSize; }

  virtual void draw() = 0;

protected:
  const cPoint mSize;
  };
//}}}
//{{{
class cTarget {
// target framebuffer object wrapper
public:
  enum eFormat { eRGB, eRGBA };

  cTarget (const cPoint& size) : mSize(size) {}
  virtual ~cTarget() = default;

  /// gets
  const cPoint getSize() const { return mSize; }
  unsigned getId() { return mFrameBufferObject; }
  unsigned getTextureId() { return mColorTextureId; }
  unsigned getNumPixels() { return mSize.x * mSize.y; }
  unsigned getNumPixelBytes() { return  mSize.x * mSize.y * 4; }

  virtual uint8_t* getPixels() = 0;

  // sets
  virtual void setSize (const cPoint& size) = 0;
  virtual void setTarget (const cRect& rect) = 0;
  virtual void setBlend() = 0;
  virtual void setSource() = 0;

  void setTarget() { setTarget (cRect (mSize)); }

  // actions
  virtual void invalidate() = 0;
  virtual void pixelsChanged (const cRect& rect) = 0;

  virtual void clear (const cColor& color) = 0;
  virtual void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) = 0;

  virtual bool checkStatus() = 0;
  virtual void reportInfo() = 0;

protected:
  cPoint mSize;
  int mImageFormat = eRGBA;
  int mInternalFormat = eRGBA;

  uint32_t mFrameBufferObject = 0;
  uint32_t mColorTextureId = 0;
  uint8_t* mPixels = nullptr;
  cRect mDirtyPixelsRect = cRect(0,0,0,0);
  };
//}}}

class cGraphics {
public:
  virtual ~cGraphics() = default;

  virtual bool init() = 0;
  virtual void newFrame() = 0;
  virtual void clear (const cPoint& size) = 0;
  virtual void renderDrawData() = 0;

  // create graphics resources
  virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) = 0;
  virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) = 0;

  virtual cQuad* createQuad (const cPoint& size) = 0;
  virtual cQuad* createQuad (const cPoint& size, const cRect& rect) = 0;

  virtual cTarget* createTarget() = 0;
  virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) = 0;
  virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) = 0;
  };

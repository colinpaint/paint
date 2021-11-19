// cGraphics.h - static register and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <map>

#include "../utils/cPointRectColor.h"
#include "../utils/cVecMat.h"

class cPlatform;
//}}}

//{{{
class cTexture {
public:
  enum eTextureType { eTextureNone, eRgba, eNv12, eYuv420 };

  cTexture (eTextureType textureType, cPoint size) : mTextureType(textureType), mSize(size) {}
  virtual ~cTexture() = default;

  /// gets
  eTextureType getTextureType() const { return mTextureType; }
  const cPoint getSize() const { return mSize; }

  unsigned getTextureId() { return mTextureId[0]; }

  virtual void setPixels (uint8_t* pixels) = 0;
  virtual void setSource() = 0;

protected:
  const eTextureType mTextureType;
  cPoint mSize;

  uint32_t mTextureId[3];
  };
//}}}
//{{{
class cQuad {
public:
  cQuad (cPoint size) : mSize(size) {}
  virtual ~cQuad() = default;

  cPoint getSize()  { return mSize; }

  virtual void draw() = 0;

protected:
  const cPoint mSize;
  };
//}}}
//{{{
class cFrameBuffer {
public:
  enum eFormat { eRGB, eRGBA };

  cFrameBuffer (cPoint size) : mSize(size) {}
  virtual ~cFrameBuffer() = default;

  /// gets
  const cPoint getSize() const { return mSize; }
  unsigned getId() { return mFrameBufferObject; }
  unsigned getTextureId() { return mColorTextureId; }
  unsigned getNumPixels() { return mSize.x * mSize.y; }
  unsigned getNumPixelBytes() { return  mSize.x * mSize.y * 4; }

  virtual uint8_t* getPixels() = 0;

  // sets
  virtual void setSize (cPoint size) = 0;
  virtual void setTarget (const cRect& rect) = 0;
  virtual void setBlend() = 0;
  virtual void setSource() = 0;

  void setTarget() { setTarget (cRect (mSize)); }

  // actions
  virtual void invalidate() = 0;
  virtual void pixelsChanged (const cRect& rect) = 0;

  virtual void clear (const cColor& color) = 0;
  virtual void blit (cFrameBuffer& src, cPoint srcPoint, const cRect& dstRect) = 0;

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

//{{{
class cShader {
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
public:
  cQuadShader() : cShader() {}
  virtual ~cQuadShader() = default;

  // sets
  virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) = 0;
  };
//}}}

//{{{
class cPaintShader : public cQuadShader {
public:
  cPaintShader() : cQuadShader() {}
  virtual ~cPaintShader() = default;

  virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) = 0;
  };
//}}}
//{{{
class cLayerShader : public cQuadShader {
public:
  cLayerShader() : cQuadShader() {}
  virtual ~cLayerShader() = default;

  virtual void setHueSatVal (float hue, float sat, float val) = 0;
  };
//}}}
//{{{
class cCanvasShader : public cQuadShader {
public:
  cCanvasShader() : cQuadShader() {}
  virtual ~cCanvasShader() = default;
  };
//}}}
//{{{
class cVideoShader : public cQuadShader {
public:
  cVideoShader() : cQuadShader() {}
  virtual ~cVideoShader() = default;

  virtual void setTextures() = 0;
  };
//}}}

class cGraphics {
public:
  // static register
  static cGraphics& createByName (const std::string& name, cPlatform& platform);
  static void listRegisteredClasses();

  // base class
  virtual void shutdown() = 0;

  // create graphics resources
  virtual cTexture* createTexture (cTexture::eTextureType textureType, cPoint size, uint8_t* pixels) = 0;

  virtual cQuad* createQuad (cPoint size) = 0;
  virtual cQuad* createQuad (cPoint size, const cRect& rect) = 0;

  virtual cFrameBuffer* createFrameBuffer() = 0;
  virtual cFrameBuffer* createFrameBuffer (cPoint size, cFrameBuffer::eFormat format) = 0;
  virtual cFrameBuffer* createFrameBuffer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format) = 0;

  virtual cPaintShader* createPaintShader() = 0;
  virtual cLayerShader* createLayerShader() = 0;
  virtual cCanvasShader* createCanvasShader() = 0;
  virtual cVideoShader* createVideoShader() = 0;

  virtual void background (int width, int height) = 0;

  // actions
  virtual void windowResize (int width, int height) = 0;
  virtual void newFrame() = 0;
  virtual void drawUI (cPoint windowSize) = 0;

protected:
  virtual bool init (cPlatform& platform) = 0;

  // static register
  using createFunc = cGraphics*(*)(const std::string& name);
  static bool registerClass (const std::string& name, const createFunc factoryMethod);

private:
  // static register
  static std::map<const std::string, createFunc>& getClassRegister();
  };

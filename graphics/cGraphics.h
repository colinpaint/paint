// cGraphics.h - graphics singleton
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// glm
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>

#include "cPointRect.h"
//}}}

// graphics resources
//{{{
class cQuad {
public:
  cQuad (cPoint size) : mSize(size) {}
  cQuad (cPoint size, const cRect& rect) : mSize(size) {}
  virtual ~cQuad() = default;

  virtual void draw() = 0;

protected:
  const cPoint mSize;

  uint32_t mVertexArrayObject = 0;
  uint32_t mVertexBufferObject = 0;
  uint32_t mElementArrayBufferObject = 0;
  uint32_t mNumIndices = 0;
  };
//}}}
//{{{
class cFrameBuffer {
public:
  enum eFormat { eRGB, eRGBA };

  cFrameBuffer (cPoint size) : mSize(size) {}
  virtual ~cFrameBuffer() = default;

  /// gets
  cPoint getSize() { return mSize; }
  unsigned getId() { return mFrameBufferObject; }
  unsigned getTextureId() { return mColorTextureId; }
  unsigned getNumPixels() { return mSize.x * mSize.y; }
  unsigned getNumPixelBytes() { return  mSize.x * mSize.y * 4; }

  virtual uint8_t* getPixels() = 0;

  //sets
  virtual void setSize (cPoint size) = 0;
  virtual void setTarget (const cRect& rect) = 0;
  virtual void setBlend() = 0;
  virtual void setSource() = 0;

  void setTarget() { setTarget (cRect (mSize)); }

  // actions
  virtual void invalidate() = 0;
  virtual void pixelsChanged (const cRect& rect) = 0;

  virtual void clear (const glm::vec4& color) = 0;
  virtual void blit (cFrameBuffer* src, cPoint srcPoint, const cRect& dstRect) = 0;

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
  cShader (const std::string& vertShaderString, const std::string& fragShaderString);
  virtual ~cShader();

  uint32_t getId() { return mId; }

  void use();

protected:
  void setBool (const std::string &name, bool value);

  void setInt (const std::string &name, int value);
  void setFloat (const std::string &name, float value);

  void setVec2 (const std::string &name, glm::vec2 value);
  void setVec3 (const std::string &name, glm::vec3 value);
  void setVec4 (const std::string &name, glm::vec4 value);

  void setMat4 (const std::string &name, glm::mat4 value);

private:
  uint32_t mId = 0;
  };
//}}}
//{{{
class cQuadShader : public cShader {
public:
  cQuadShader (const std::string& fragShaderString);
  virtual ~cQuadShader() = default;

  void setModelProject (const glm::mat4& model, const glm::mat4& project);
  };
//}}}
//{{{
class cCanvasShader : public cQuadShader {
public:
  cCanvasShader();
  virtual ~cCanvasShader() = default;
  };
//}}}
//{{{
class cLayerShader : public cQuadShader {
public:
  cLayerShader();
  virtual ~cLayerShader() = default;

  void setHueSatVal (float hue, float sat, float val);
  };
//}}}
//{{{
class cPaintShader : public cQuadShader {
public:
  cPaintShader();
  virtual ~cPaintShader() = default;

  void setStroke (const glm::vec2& pos, const glm::vec2& prevPos, float radius, const glm::vec4& color);
  };
//}}}

class cGraphics {
public:
  //{{{
  static cGraphics& getInstance() {
  // singleton pattern create
  // - thread safe
  // - allocate with `new` in case singleton is not trivially destructible.

    static cGraphics* graphics = new cGraphics();
    return *graphics;
    }
  //}}}

  bool init (void* device, void* deviceContext, void* swapChain);
  void shutdown();

  // create graphics resources
  cQuad* createQuad (cPoint size);
  cQuad* createQuad (cPoint size, const cRect& rect);

  cFrameBuffer* createFrameBuffer();
  cFrameBuffer* createFrameBuffer (cPoint size, cFrameBuffer::eFormat format);
  cFrameBuffer* createFrameBuffer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format);

  cCanvasShader* createCanvasShader();
  cLayerShader* createLayerShader();
  cPaintShader* createPaintShader();

  // actions
  void draw();
  void windowResized (int width, int height);

private:
  // singleton pattern fluff
  cGraphics() = default;

  // delete copy/move so extra instances can't be created/moved
  cGraphics (const cGraphics&) = delete;
  cGraphics& operator = (const cGraphics&) = delete;
  cGraphics (cGraphics&&) = delete;
  cGraphics& operator = (cGraphics&&) = delete;
  };

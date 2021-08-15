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

class cGraphics {
public:
  //{{{
  class cQuad {
  public:
    cQuad (cPoint size) : mSize(size) {}
    cQuad (cPoint size, const cRect& rect) : mSize(size) {}
    virtual ~cQuad() = default;

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
    cShader() = default;
    virtual ~cShader() = default;

    uint32_t getId() { return mId; }

    virtual void use() = 0;

  protected:
    uint32_t mId = 0;
    };
  //}}}
  //{{{
  class cPaintShader : public cShader {
  public:
    cPaintShader() = default;
    virtual ~cPaintShader() = default;

    virtual void setModelProject (const glm::mat4& model, const glm::mat4& project) = 0;
    virtual void setStroke (const glm::vec2& pos, const glm::vec2& prevPos, float radius, const glm::vec4& color) = 0;
    };
  //}}}
  //{{{
  class cLayerShader : public cShader {
  public:
    cLayerShader() = default;
    virtual ~cLayerShader() = default;

    virtual void setModelProject (const glm::mat4& model, const glm::mat4& project) = 0;
    virtual void setHueSatVal (float hue, float sat, float val) = 0;
    };
  //}}}
  //{{{
  class cCanvasShader : public cShader {
  public:
    cCanvasShader() = default;
    virtual ~cCanvasShader() = default;

    virtual void setModelProject (const glm::mat4& model, const glm::mat4& project) = 0;
    };
  //}}}

  // factory create
  static cGraphics& create();

  cGraphics() = default;
  virtual ~cGraphics() = default;

  virtual bool init (void* device, void* deviceContext, void* swapChain) = 0;
  virtual void shutdown() = 0;

  // create graphics resources
  virtual cQuad* createQuad (cPoint size) = 0;
  virtual cQuad* createQuad (cPoint size, const cRect& rect) = 0;

  virtual cFrameBuffer* createFrameBuffer() = 0;
  virtual cFrameBuffer* createFrameBuffer (cPoint size, cFrameBuffer::eFormat format) = 0;
  virtual cFrameBuffer* createFrameBuffer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format) = 0;

  virtual cCanvasShader* createCanvasShader() = 0;
  virtual cLayerShader* createLayerShader() = 0;
  virtual cPaintShader* createPaintShader() = 0;

  // actions
  virtual void draw() = 0;
  virtual void windowResized (int width, int height) = 0;
  };

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

//{{{
class cQuad {
public:
  cQuad (cPoint size);
  cQuad (cPoint size, const cRect& rect);
  ~cQuad();

  void draw();

private:
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
//{{{
class cDrawListShader : public cShader {
public:
  cDrawListShader (uint32_t glslVersion);
  virtual ~cDrawListShader() = default;

  int32_t getAttribLocationVtxPos() { return mAttribLocationVtxPos; }
  int32_t getAttribLocationVtxUV() { return mAttribLocationVtxUV; }
  int32_t getAttribLocationVtxColor() { return mAttribLocationVtxColor; }

  void setMatrix (float* matrix);

private:
  int32_t mAttribLocationTexture = 0;
  int32_t mAttribLocationProjMtx = 0;

  int32_t mAttribLocationVtxPos = 0;
  int32_t mAttribLocationVtxUV = 0;
  int32_t mAttribLocationVtxColor = 0;
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

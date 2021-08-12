// cFrameBuffer.cpp - FrameBufferObject wrapper
//{{{  includes
#include "cFrameBuffer.h"

#include <cstdint>
#include <cmath>
#include <string>

// glad
#include <glad/glad.h>

#include "cPointRect.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}
constexpr bool kDebug = false;

//{{{
cFrameBuffer::cFrameBuffer()
    : mImageFormat(GL_RGBA), mInternalFormat(GL_RGBA) {
// window frameBuffer
  }
//}}}
//{{{
cFrameBuffer::cFrameBuffer (cPoint size, eFormat format)
    : mSize(size),
      mImageFormat(format == eRGBA ? GL_RGBA : GL_RGB),
      mInternalFormat(format == eRGBA ? GL_RGBA : GL_RGB) {

  // create empty frameBuffer object
  //glGenFramebuffers (1, &mFrameBufferObject);
  //glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

  // create and add texture to frameBuffer object
  glGenTextures (1, &mColorTextureId);

  glBindTexture (GL_TEXTURE_2D, mColorTextureId);
  glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, 0);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glGenerateMipmap (GL_TEXTURE_2D);

  //glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
  }
//}}}
//{{{
cFrameBuffer::cFrameBuffer (uint8_t* pixels, cPoint size, eFormat format)
    : mSize(size),
      mImageFormat(format == eRGBA ? GL_RGBA : GL_RGB),
      mInternalFormat(format == eRGBA ? GL_RGBA : GL_RGB) {

  // create frameBuffer object from pixels
  //glGenFramebuffers (1, &mFrameBufferObject);
  //glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

  // create and add texture to frameBuffer object
  glGenTextures (1, &mColorTextureId);

  glBindTexture (GL_TEXTURE_2D, mColorTextureId);
  glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, pixels);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  //glGenerateMipmap (GL_TEXTURE_2D);

  //glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
  }
//}}}
//{{{
cFrameBuffer::~cFrameBuffer() {

  glDeleteTextures (1, &mColorTextureId);
  //glDeleteFramebuffers (1, &mFrameBufferObject);
  free (mPixels);
  }
//}}}

//{{{
uint8_t* cFrameBuffer::getPixels() {

  if (!mPixels) {
    // create mPixels, texture pixels shadow buffer
    if (kDebug)
      cLog::log (LOGINFO, format ("getPixels malloc {}", getNumPixelBytes()));
    mPixels = static_cast<uint8_t*>(malloc (getNumPixelBytes()));
    glBindTexture (GL_TEXTURE_2D, mColorTextureId);
    glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
    }

  else if (!mDirtyPixelsRect.isEmpty()) {
    if (kDebug)
      cLog::log (LOGINFO, format ("getPixels get {},{} {},{}",
                                  mDirtyPixelsRect.left, mDirtyPixelsRect.top,
                                  mDirtyPixelsRect.getWidth(), mDirtyPixelsRect.getHeight()));

    // no openGL glGetTexSubImage, so dirtyPixelsRect not really used, is this correct ???
    glBindTexture (GL_TEXTURE_2D, mColorTextureId);
    glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
    mDirtyPixelsRect = cRect(0,0,0,0);
    }

  return mPixels;
  }
//}}}

//{{{
void cFrameBuffer::setSize (cPoint size) {

  if (mFrameBufferObject == 0)
    mSize = size;
  else
    cLog::log (LOGERROR, "unimplmented setSize of non screen framebuffer");
  };
//}}}
//{{{
void cFrameBuffer::setTarget (const cRect& rect) {
// set us as target, set viewport to our size, invalidate contents (probably a clear)

  //glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);
  glViewport (0, 0, mSize.x, mSize.y);

  glDisable (GL_SCISSOR_TEST);
  glDisable (GL_CULL_FACE);
  glDisable (GL_DEPTH_TEST);

  // texture could be changed, add to dirtyPixelsRect
  mDirtyPixelsRect += rect;
  }
//}}}
//{{{
void cFrameBuffer::setBlend() {

  uint32_t modeRGB = GL_FUNC_ADD;
  uint32_t modeAlpha = GL_FUNC_ADD;

  uint32_t srcRgb = GL_SRC_ALPHA;
  uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
  uint32_t srcAlpha = GL_ONE;
  uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;

  glBlendEquationSeparate (modeRGB, modeAlpha);
  glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);
  glEnable (GL_BLEND);
  }
//}}}
//{{{
void cFrameBuffer::setSource() {

  if (mFrameBufferObject) {
    glActiveTexture (GL_TEXTURE0);
    glBindTexture (GL_TEXTURE_2D, mColorTextureId);
    }
  else {
    //cLog::log (LOGERROR, "windowFrameBuffer cannot be src");
    }
  }
//}}}

//{{{
void cFrameBuffer::invalidate() {

  //glInvalidateFramebuffer (mFrameBufferObject, 1, GL_COLOR_ATTACHMENT0);
  clear (glm::vec4 (0.f,0.f,0.f, 0.f));
  }
//}}}
//{{{
void cFrameBuffer::pixelsChanged (const cRect& rect) {
// pixels in rect changed, write back to texture

  if (mPixels) {
    // update y band, quicker x cropped line by line ???
    glBindTexture (GL_TEXTURE_2D, mColorTextureId);
    glTexSubImage2D (GL_TEXTURE_2D, 0, 0, rect.top, mSize.x, rect.getHeight(),
                     mImageFormat, GL_UNSIGNED_BYTE, mPixels + (rect.top * mSize.x * 4));
    // simpler whole screen
    //glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, mSize.x, mSize.y, 0, mImageFormat, GL_UNSIGNED_BYTE, mPixels);

    if (kDebug)
      cLog::log (LOGINFO, format ("pixelsChanged {},{} {},{} - dirty {},{} {},{}",
                                  rect.left, rect.top, rect.getWidth(), rect.getHeight()));
    }
  }
//}}}

//{{{
void cFrameBuffer::clear (const glm::vec4& color) {

  glClearColor (color.x, color.y, color.z, color.w);
  glClear (GL_COLOR_BUFFER_BIT);
  }
//}}}
//{{{
void cFrameBuffer::blit (cFrameBuffer* src, cPoint srcPoint, const cRect& dstRect) {

  // no OpenGL 2.1 blit
  //glBindFramebuffer (GL_READ_FRAMEBUFFER, src->getId());
  //glBindFramebuffer (GL_DRAW_FRAMEBUFFER, mFrameBufferObject);
  //glBlitFramebuffer (srcPoint.x, srcPoint.y, srcPoint.x + dstRect.getWidth(), srcPoint.y + dstRect.getHeight(),
  //                   dstRect.left, dstRect.top, dstRect.right, dstRect.bottom,
  //                   GL_COLOR_BUFFER_BIT, GL_NEAREST);

  // texture changed, add to dirtyPixelsRect
  mDirtyPixelsRect += dstRect;

  if (kDebug)
    cLog::log (LOGINFO, format ("blit src:{},{} dst:{},{} {},{} dirty:{},{} {},{}",
                                srcPoint.x, srcPoint.y,
                                dstRect.left, dstRect.top, dstRect.getWidth(), dstRect.getHeight(),
                                mDirtyPixelsRect.left, mDirtyPixelsRect.top,
                                mDirtyPixelsRect.getWidth(), mDirtyPixelsRect.getHeight()));
  }
//}}}

//{{{
bool cFrameBuffer::checkStatus() {
// none to check
  return true;
  }
//}}}
//{{{
void cFrameBuffer::reportInfo() {
// none to report
  }
//}}}

// private
//{{{
string cFrameBuffer::getInternalFormat (uint32_t formatNum) {
  return ""
  }
//}}}
//{{{
string cFrameBuffer::getTextureParameters (uint32_t id) {

  if (glIsTexture(id) == GL_FALSE)
    return "Not texture object";

  int width, height, formatNum;
  glBindTexture (GL_TEXTURE_2D, id);
  glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
  glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
  glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &formatNum); // get texture internal format

  return format (" {} {} {}", width, height, getInternalFormat (formatNum));
  }
//}}}
//{{{
string cFrameBuffer::getRenderbufferParameters (uint32_t id) {
  return getTextureParameters (id);
  }
//}}}

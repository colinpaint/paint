// cFrameBuffer-DirectX11.cpp - FrameBufferObject wrapper - NOT YET
//{{{  includes
#include "cFrameBuffer.h"

#include <cstdint>
#include <cmath>
#include <string>

#include "cPointRect.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}
constexpr bool kDebug = false;

//{{{
cFrameBuffer::cFrameBuffer()
    : mImageFormat(0), mInternalFormat(0) {
// window frameBuffer
  }
//}}}
//{{{
cFrameBuffer::cFrameBuffer (cPoint size, eFormat format)
    : mSize(size),
      mImageFormat(format == eRGBA ? 0 : 1),
      mInternalFormat(format == eRGBA ? 0 : 1) {

  // create empty frameBuffer
  }
//}}}
//{{{
cFrameBuffer::cFrameBuffer (uint8_t* pixels, cPoint size, eFormat format)
    : mSize(size),
      mImageFormat(format == eRGBA ? 0 : 1),
      mInternalFormat(format == eRGBA ? 0 : 1) {

  // create frameBuffer from pixels
  }
//}}}
//{{{
cFrameBuffer::~cFrameBuffer() {

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
    }

  else if (!mDirtyPixelsRect.isEmpty()) {
    if (kDebug)
      cLog::log (LOGINFO, format ("getPixels get {},{} {},{}",
                                  mDirtyPixelsRect.left, mDirtyPixelsRect.top,
                                  mDirtyPixelsRect.getWidth(), mDirtyPixelsRect.getHeight()));

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

  // texture could be changed, add to dirtyPixelsRect
  mDirtyPixelsRect += rect;
  }
//}}}
//{{{
void cFrameBuffer::setBlend() {
  }
//}}}
//{{{
void cFrameBuffer::setSource() {

  if (mFrameBufferObject) {
    }
  else
    cLog::log (LOGERROR, "windowFrameBuffer cannot be src");
  }
//}}}

//{{{
void cFrameBuffer::invalidate() {

  clear (glm::vec4 (0.f,0.f,0.f, 0.f));
  }
//}}}
//{{{
void cFrameBuffer::pixelsChanged (const cRect& rect) {
// pixels in rect changed, write back to texture

  if (mPixels) {
    if (kDebug)
      cLog::log (LOGINFO, format ("pixelsChanged {},{} {},{} - dirty {},{} {},{}",
                                  rect.left, rect.top, rect.getWidth(), rect.getHeight()));
    }
  }
//}}}

void cFrameBuffer::clear (const glm::vec4& color) {}
//{{{
void cFrameBuffer::blit (cFrameBuffer* src, cPoint srcPoint, const cRect& dstRect) {

  mDirtyPixelsRect += dstRect;

  if (kDebug)
    cLog::log (LOGINFO, format ("blit src:{},{} dst:{},{} {},{} dirty:{},{} {},{}",
                                srcPoint.x, srcPoint.y,
                                dstRect.left, dstRect.top, dstRect.getWidth(), dstRect.getHeight(),
                                mDirtyPixelsRect.left, mDirtyPixelsRect.top,
                                mDirtyPixelsRect.getWidth(), mDirtyPixelsRect.getHeight()));
  }
//}}}

bool cFrameBuffer::checkStatus() { return true; }
//{{{
void cFrameBuffer::reportInfo() { 
  cLog::log (LOGINFO, format ("frameBuffer reportInfo {},{}", mSize.x, mSize.y));
  }
//}}}

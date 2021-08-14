// cFrameBuffer-OpenGL.cpp - FrameBufferObject wrapper
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
namespace {
  //{{{
  string getInternalFormat (uint32_t formatNum) {

    string formatName;

    switch (formatNum) {
      case GL_STENCIL_INDEX:      // 0x1901
        formatName = "GL_STENCIL_INDEX";
        break;
      case GL_DEPTH_COMPONENT:    // 0x1902
        formatName = "GL_DEPTH_COMPONENT";
        break;
      case GL_ALPHA:              // 0x1906
        formatName = "GL_ALPHA";
        break;
      case GL_RGB:                // 0x1907
        formatName = "GL_RGB";
        break;
      case GL_RGBA:               // 0x1908
        formatName = "GL_RGBA";
        break;
      //case GL_LUMINANCE:          // 0x1909
      //  formatName = "GL_LUMINANCE";
      //  break;
      //case GL_LUMINANCE_ALPHA:    // 0x190A
      //  formatName = "GL_LUMINANCE_ALPHA";
      //  break;
      case GL_R3_G3_B2:           // 0x2A10
        formatName = "GL_R3_G3_B2";
        break;
      case GL_RGB4:               // 0x804F
          formatName = "GL_RGB4";
          break;
      case GL_RGB5:               // 0x8050
          formatName = "GL_RGB5";
          break;
      case GL_RGB8:               // 0x8051
          formatName = "GL_RGB8";
          break;
      case GL_RGB10:              // 0x8052
          formatName = "GL_RGB10";
          break;
      case GL_RGB12:              // 0x8053
          formatName = "GL_RGB12";
          break;
      case GL_RGB16:              // 0x8054
          formatName = "GL_RGB16";
          break;
      case GL_RGBA2:              // 0x8055
          formatName = "GL_RGBA2";
          break;
      case GL_RGBA4:              // 0x8056
          formatName = "GL_RGBA4";
          break;
      case GL_RGB5_A1:            // 0x8057
          formatName = "GL_RGB5_A1";
          break;
      case GL_RGBA8:              // 0x8058
          formatName = "GL_RGBA8";
          break;
      case GL_RGB10_A2:           // 0x8059
          formatName = "GL_RGB10_A2";
          break;
      case GL_RGBA12:             // 0x805A
          formatName = "GL_RGBA12";
          break;
      case GL_RGBA16:             // 0x805B
          formatName = "GL_RGBA16";
          break;
      case GL_DEPTH_COMPONENT16:  // 0x81A5
          formatName = "GL_DEPTH_COMPONENT16";
          break;
      case GL_DEPTH_COMPONENT24:  // 0x81A6
          formatName = "GL_DEPTH_COMPONENT24";
          break;
      case GL_DEPTH_COMPONENT32:  // 0x81A7
          formatName = "GL_DEPTH_COMPONENT32";
          break;
      case GL_DEPTH_STENCIL:      // 0x84F9
          formatName = "GL_DEPTH_STENCIL";
          break;
      case GL_DEPTH24_STENCIL8:   // 0x88F0
          formatName = "GL_DEPTH24_STENCIL8";
          break;
      // openGL v4? onwards
      #ifdef OPENGL_RGBF
      case GL_RGBA32F:            // 0x8814
        formatName = "GL_RGBA32F";
        break;
      case GL_RGB32F:             // 0x8815
        formatName = "GL_RGB32F";
        break;
      case GL_RGBA16F:            // 0x881A
        formatName = "GL_RGBA16F";
        break;
      case GL_RGB16F:             // 0x881B
        formatName = "GL_RGB16F";
        break;
      #endif
      default:
          formatName = format ("Unknown Format {}", formatNum);
          break;
      }

    return formatName;
    }
  //}}}
  //{{{
  string getTextureParameters (uint32_t id) {

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
  string getRenderbufferParameters (uint32_t id) {

    if (glIsRenderbuffer(id) == GL_FALSE)
      return "Not Renderbuffer object";

    int width, height, formatNum, samples;
    glBindRenderbuffer (GL_RENDERBUFFER, id);
    glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);    // get renderbuffer width
    glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);  // get renderbuffer height
    glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &formatNum); // get renderbuffer internal format
    glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);   // get multisample count

    return format (" {} {} {} {}", width, height, samples, getInternalFormat (formatNum));
    }
  //}}}
  }

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
  glGenFramebuffers (1, &mFrameBufferObject);
  glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

  // create and add texture to frameBuffer object
  glGenTextures (1, &mColorTextureId);

  glBindTexture (GL_TEXTURE_2D, mColorTextureId);
  glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, 0);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap (GL_TEXTURE_2D);

  glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
  }
//}}}
//{{{
cFrameBuffer::cFrameBuffer (uint8_t* pixels, cPoint size, eFormat format)
    : mSize(size),
      mImageFormat(format == eRGBA ? GL_RGBA : GL_RGB),
      mInternalFormat(format == eRGBA ? GL_RGBA : GL_RGB) {

  // create frameBuffer object from pixels
  glGenFramebuffers (1, &mFrameBufferObject);
  glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

  // create and add texture to frameBuffer object
  glGenTextures (1, &mColorTextureId);

  glBindTexture (GL_TEXTURE_2D, mColorTextureId);
  glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, size.x, size.y, 0, mImageFormat, GL_UNSIGNED_BYTE, pixels);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGenerateMipmap (GL_TEXTURE_2D);

  glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTextureId, 0);
  }
//}}}
//{{{
cFrameBuffer::~cFrameBuffer() {

  glDeleteTextures (1, &mColorTextureId);
  glDeleteFramebuffers (1, &mFrameBufferObject);
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

  glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);
  glViewport (0, 0, mSize.x, mSize.y);

  glDisable (GL_SCISSOR_TEST);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);

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
  else
    cLog::log (LOGERROR, "windowFrameBuffer cannot be src");
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

  glBindFramebuffer (GL_READ_FRAMEBUFFER, src->getId());
  glBindFramebuffer (GL_DRAW_FRAMEBUFFER, mFrameBufferObject);
  glBlitFramebuffer (srcPoint.x, srcPoint.y, srcPoint.x + dstRect.getWidth(), srcPoint.y + dstRect.getHeight(),
                     dstRect.left, dstRect.top, dstRect.right, dstRect.bottom,
                     GL_COLOR_BUFFER_BIT, GL_NEAREST);

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

  GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

  switch (status) {
    case GL_FRAMEBUFFER_COMPLETE:
      //cLog::log (LOGINFO, "frameBUffer ok");
      return true;

    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      cLog::log (LOGERROR, "framebuffer incomplete: Attachment is NOT complete");
      return false;

    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      cLog::log (LOGERROR, "framebuffer incomplete: No image is attached to FBO");
      return false;

    //case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
    //  cLog::log (LOGERROR, "framebuffer incomplete: Attached images have different dimensions");
    //  return false;

    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
      cLog::log (LOGERROR, "framebuffer incomplete: Draw buffer");
      return false;

    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
      cLog::log (LOGERROR, "framebuffer incomplete: Read buffer");
      return false;

    case GL_FRAMEBUFFER_UNSUPPORTED:
      cLog::log (LOGERROR, "framebuffer incomplete: Unsupported by FBO implementation");
      return false;

    default:
      cLog::log (LOGERROR, "framebuffer incomplete: Unknown error");
      return false;
    }
  }
//}}}
//{{{
void cFrameBuffer::reportInfo() {

  glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

  GLint colorBufferCount = 0;
  glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, &colorBufferCount);

  GLint multiSampleCount = 0;
  glGetIntegerv (GL_MAX_SAMPLES, &multiSampleCount);

  cLog::log (LOGINFO, format ("frameBuffer maxColorAttach {} masSamples {}", colorBufferCount, multiSampleCount));

  //  print info of the colorbuffer attachable image
  GLint objectType;
  GLint objectId;
  for (int i = 0; i < colorBufferCount; ++i) {
    glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                           GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
    if (objectType != GL_NONE) {
      glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                             GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
      string formatName;
      cLog::log (LOGINFO, format ("- color{}", i));
      if (objectType == GL_TEXTURE)
        cLog::log (LOGINFO, format ("  - GL_TEXTURE {}", getTextureParameters (objectId)));
      else if(objectType == GL_RENDERBUFFER)
        cLog::log (LOGINFO, format ("  - GL_RENDERBUFFER {}", getRenderbufferParameters (objectId)));
      }
    }

  //  print info of the depthbuffer attachable image
  glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                         GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
  if (objectType != GL_NONE) {
    glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                           GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
    cLog::log (LOGINFO, format ("depth"));
    switch (objectType) {
      case GL_TEXTURE:
        cLog::log (LOGINFO, format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
        break;
      case GL_RENDERBUFFER:
        cLog::log (LOGINFO, format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
        break;
      }
    }

  // print info of the stencilbuffer attachable image
  glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                        &objectType);
  if (objectType != GL_NONE) {
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
    cLog::log (LOGINFO, format ("stencil"));
    switch (objectType) {
      case GL_TEXTURE:
        cLog::log (LOGINFO, format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
        break;
      case GL_RENDERBUFFER:
        cLog::log (LOGINFO, format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
        break;
      }
    }
  }
//}}}

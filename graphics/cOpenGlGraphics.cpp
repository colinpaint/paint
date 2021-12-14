// cOpenGlGraphics.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <cmath>
#include <string>
#include <array>
#include <algorithm>

// glad
#include <glad/glad.h>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_opengl3.h>

#include "cGraphics.h"
#include "../utils/cLog.h"

// OpenGL >= 3.1 has GL_PRIMITIVE_RESTART state
// OpenGL >= 3.3 has glBindSampler()
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
  #if defined(GL_VERSION_3_2)
    // OpenGL >= 3.2 has glDrawElementsBaseVertex() which GL ES and WebGL don't have.
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
  #endif
#endif

using namespace std;
//}}}
//#define USE_IMPL // use imgui backend impl untouched, !!! not working yet !!!
constexpr bool kDebug = false;

namespace {
  //{{{
  class cOpenGlQuad : public cQuad {
  public:
    //{{{
    cOpenGlQuad (const cPoint& size) : cQuad(size) {

      // vertexArray
      glGenVertexArrays (1, &mVertexArrayObject);
      glBindVertexArray (mVertexArrayObject);

      // vertices
      glGenBuffers (1, &mVertexBufferObject);
      glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

      glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
      glEnableVertexAttribArray (0);

      glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
      glEnableVertexAttribArray (1);

      const float widthF = static_cast<float>(size.x);
      const float heightF = static_cast<float>(size.y);
      const float kVertices[] = {
        0.f,   heightF,  0.f,1.f, // tl vertex
        widthF,0.f,      1.f,0.f, // br vertex
        0.f,   0.f,      0.f,0.f, // bl vertex
        widthF,heightF,  1.f,1.f, // tr vertex
        };

      glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

      glGenBuffers (1, &mElementArrayBufferObject);
      glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);

      glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
      }
    //}}}
    //{{{
    cOpenGlQuad (const cPoint& size, const cRect& rect) : cQuad(size) {

      // vertexArray
      glGenVertexArrays (1, &mVertexArrayObject);
      glBindVertexArray (mVertexArrayObject);

      // vertices
      glGenBuffers (1, &mVertexBufferObject);
      glBindBuffer (GL_ARRAY_BUFFER, mVertexBufferObject);

      glVertexAttribPointer (0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
      glEnableVertexAttribArray (0);

      glVertexAttribPointer (1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
      glEnableVertexAttribArray (1);

      const float subLeftF = static_cast<float>(rect.left);
      const float subRightF = static_cast<float>(rect.right);
      const float subBottomF = static_cast<float>(rect.bottom);
      const float subTopF = static_cast<float>(rect.top);

      const float subLeftTexF = subLeftF / size.x;
      const float subRightTexF = subRightF / size.x;
      const float subBottomTexF = subBottomF / size.y;
      const float subTopTexF = subTopF / size.y;

      const float kVertices[] = {
        subLeftF, subTopF,     subLeftTexF, subTopTexF,    // tl
        subRightF,subBottomF,  subRightTexF,subBottomTexF, // br
        subLeftF, subBottomF,  subLeftTexF, subBottomTexF, // bl
        subRightF,subTopF,     subRightTexF,subTopTexF     // tr
        };

      glBufferData (GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);

      // indices
      glGenBuffers (1, &mElementArrayBufferObject);
      glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, mElementArrayBufferObject);
      glBufferData (GL_ELEMENT_ARRAY_BUFFER, mNumIndices, kIndices, GL_STATIC_DRAW);
      }
    //}}}
    //{{{
    virtual ~cOpenGlQuad() {

      glDeleteBuffers (1, &mElementArrayBufferObject);
      glDeleteBuffers (1, &mVertexBufferObject);
      glDeleteVertexArrays (1, &mVertexArrayObject);
      }
    //}}}

    //{{{
    void draw() final {

      glBindVertexArray (mVertexArrayObject);
      glDrawElements (GL_TRIANGLES, mNumIndices, GL_UNSIGNED_BYTE, 0);
      }
    //}}}

  private:
    inline static const uint32_t mNumIndices = 6;
    //{{{
    inline static const uint8_t kIndices[mNumIndices] = {
      0, 1, 2, // 0   0-3
      0, 3, 1  // |\   \|
      };       // 2-1   1
    //}}}

    uint32_t mVertexArrayObject = 0;
    uint32_t mVertexBufferObject = 0;
    uint32_t mElementArrayBufferObject = 0;
    };
  //}}}
  //{{{
  class cOpenGlFrameBuffer : public cFrameBuffer {
  public:
    //{{{
    cOpenGlFrameBuffer() : cFrameBuffer ({0,0}) {
    // window frameBuffer

      mImageFormat = GL_RGBA;
      mInternalFormat = GL_RGBA;
      }
    //}}}
    //{{{
    cOpenGlFrameBuffer (const cPoint& size, eFormat format) : cFrameBuffer(size) {

      mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
      mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

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
    cOpenGlFrameBuffer (uint8_t* pixels, const cPoint& size, eFormat format) : cFrameBuffer(size) {

      mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
      mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

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
    virtual ~cOpenGlFrameBuffer() {

      glDeleteTextures (1, &mColorTextureId);
      glDeleteFramebuffers (1, &mFrameBufferObject);
      free (mPixels);
      }
    //}}}

    /// gets
    //{{{
    uint8_t* getPixels() final {

      if (!mPixels) {
        // create mPixels, texture pixels shadow buffer
        if (kDebug)
          cLog::log (LOGINFO, fmt::format ("getPixels malloc {}", getNumPixelBytes()));
        mPixels = static_cast<uint8_t*>(malloc (getNumPixelBytes()));
        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
        }

      else if (!mDirtyPixelsRect.isEmpty()) {
        if (kDebug)
          cLog::log (LOGINFO, fmt::format ("getPixels get {},{} {},{}",
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

    // sets
    //{{{
    void setSize (const cPoint& size) final {
      if (mFrameBufferObject == 0)
        mSize = size;
      else
        cLog::log (LOGERROR, "unimplmented setSize of non screen framebuffer");
      };
    //}}}
    //{{{
    void setTarget (const cRect& rect) final {
    // set us as target, set viewport to our size, invalidate contents (probably a clear)

      glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);
      glViewport (0, 0, mSize.x, mSize.y);

      glDisable (GL_SCISSOR_TEST);
      glDisable (GL_CULL_FACE);
      glDisable (GL_DEPTH_TEST);

      // texture could be changed, add to dirtyPixelsRect
      mDirtyPixelsRect += rect;
      }
    //}}}
    //{{{
    void setBlend() final {

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
    void setSource() final {

      if (mFrameBufferObject) {
        glActiveTexture (GL_TEXTURE0);
        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        }
      else
        cLog::log (LOGERROR, "windowFrameBuffer cannot be src");
      }
    //}}}

    // actions
    //{{{
    void invalidate() final {

      //glInvalidateFramebuffer (mFrameBufferObject, 1, GL_COLOR_ATTACHMENT0);
      clear (cColor(0.f,0.f,0.f, 0.f));
      }
    //}}}
    //{{{
    void pixelsChanged (const cRect& rect) final {
    // pixels in rect changed, write back to texture

      if (mPixels) {
        // update y band, quicker x cropped line by line ???
        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        glTexSubImage2D (GL_TEXTURE_2D, 0, 0, rect.top, mSize.x, rect.getHeight(),
                         mImageFormat, GL_UNSIGNED_BYTE, mPixels + (rect.top * mSize.x * 4));
        // simpler whole screen
        //glTexImage2D (GL_TEXTURE_2D, 0, mInternalFormat, mSize.x, mSize.y, 0, mImageFormat, GL_UNSIGNED_BYTE, mPixels);

        if (kDebug)
          cLog::log (LOGINFO, fmt::format ("pixelsChanged {},{} {},{} - dirty {},{} {},{}",
                                      rect.left, rect.top, rect.getWidth(), rect.getHeight()));
        }
      }
    //}}}

    //{{{
    void clear (const cColor& color) final {
      glClearColor (color.r,color.g,color.b, color.a);
      glClear (GL_COLOR_BUFFER_BIT);
      }
    //}}}
    //{{{
    void blit (cFrameBuffer& src, const cPoint& srcPoint, const cRect& dstRect) final {

      glBindFramebuffer (GL_READ_FRAMEBUFFER, src.getId());
      glBindFramebuffer (GL_DRAW_FRAMEBUFFER, mFrameBufferObject);
      glBlitFramebuffer (srcPoint.x, srcPoint.y, srcPoint.x + dstRect.getWidth(), srcPoint.y + dstRect.getHeight(),
                         dstRect.left, dstRect.top, dstRect.right, dstRect.bottom,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);

      // texture changed, add to dirtyPixelsRect
      mDirtyPixelsRect += dstRect;

      if (kDebug)
        cLog::log (LOGINFO, fmt::format ("blit src:{},{} dst:{},{} {},{} dirty:{},{} {},{}",
                                    srcPoint.x, srcPoint.y,
                                    dstRect.left, dstRect.top, dstRect.getWidth(), dstRect.getHeight(),
                                    mDirtyPixelsRect.left, mDirtyPixelsRect.top,
                                    mDirtyPixelsRect.getWidth(), mDirtyPixelsRect.getHeight()));
      }
    //}}}

    //{{{
    bool checkStatus() final {

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
    void reportInfo() final {

      glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

      GLint colorBufferCount = 0;
      glGetIntegerv (GL_MAX_COLOR_ATTACHMENTS, &colorBufferCount);

      GLint multiSampleCount = 0;
      glGetIntegerv (GL_MAX_SAMPLES, &multiSampleCount);

      cLog::log (LOGINFO, fmt::format ("frameBuffer maxColorAttach {} masSamples {}", colorBufferCount, multiSampleCount));

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
          cLog::log (LOGINFO, fmt::format ("- color{}", i));
          if (objectType == GL_TEXTURE)
            cLog::log (LOGINFO, fmt::format ("  - GL_TEXTURE {}", getTextureParameters (objectId)));
          else if(objectType == GL_RENDERBUFFER)
            cLog::log (LOGINFO, fmt::format ("  - GL_RENDERBUFFER {}", getRenderbufferParameters (objectId)));
          }
        }

      //  print info of the depthbuffer attachable image
      glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                             GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
      if (objectType != GL_NONE) {
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
        cLog::log (LOGINFO, fmt::format ("depth"));
        switch (objectType) {
          case GL_TEXTURE:
            cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
            break;
          case GL_RENDERBUFFER:
            cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
            break;
          }
        }

      // print info of the stencilbuffer attachable image
      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                                            &objectType);
      if (objectType != GL_NONE) {
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
        cLog::log (LOGINFO, fmt::format ("stencil"));
        switch (objectType) {
          case GL_TEXTURE:
            cLog::log (LOGINFO, fmt::format ("- GL_TEXTURE {}", getTextureParameters(objectId)));
            break;
          case GL_RENDERBUFFER:
            cLog::log (LOGINFO, fmt::format ("- GL_RENDERBUFFER {}", getRenderbufferParameters(objectId)));
            break;
          }
        }
      }
    //}}}

  private:
    //{{{
    static string getInternalFormat (uint32_t formatNum) {

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
            formatName = fmt::format ("Unknown Format {}", formatNum);
            break;
        }

      return formatName;
      }
    //}}}
    //{{{
    static string getTextureParameters (uint32_t id) {

      if (glIsTexture(id) == GL_FALSE)
        return "Not texture object";

      int width, height, formatNum;
      glBindTexture (GL_TEXTURE_2D, id);
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
      glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &formatNum); // get texture internal format

      return fmt::format (" {} {} {}", width, height, getInternalFormat (formatNum));
      }
    //}}}
    //{{{
    static string getRenderbufferParameters (uint32_t id) {

      if (glIsRenderbuffer(id) == GL_FALSE)
        return "Not Renderbuffer object";

      int width, height, formatNum, samples;
      glBindRenderbuffer (GL_RENDERBUFFER, id);
      glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);    // get renderbuffer width
      glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);  // get renderbuffer height
      glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &formatNum); // get renderbuffer internal format
      glGetRenderbufferParameteriv (GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);   // get multisample count

      return fmt::format (" {} {} {} {}", width, height, samples, getInternalFormat (formatNum));
      }
    //}}}
    };
  //}}}

  // shader
  //{{{
  const string kQuadVertShader =

    #ifdef OPENGL_21
      "#version 120\n"
      "layout (location = 0) in vec2 inPos;"
      "layout (location = 1) in vec2 inTextureCoord;"
      "out vec2 textureCoord;"

      "uniform mat4 uModel;"
      "uniform mat4 uProject;"

      "void main() {"
      "  textureCoord = inTextureCoord;"
      "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
      "  }";

    #else
      "#version 330 core\n"
      "layout (location = 0) in vec2 inPos;"
      "layout (location = 1) in vec2 inTextureCoord;"
      "out vec2 textureCoord;"

      "uniform mat4 uModel;"
      "uniform mat4 uProject;"

      "void main() {"
      "  textureCoord = inTextureCoord;"
      "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
      "  }";

    #endif
  //}}}
  //{{{
  uint32_t compileShader (const string& vertShaderString, const string& fragShaderString) {

    // compile vertShader
    const GLuint vertShader = glCreateShader (GL_VERTEX_SHADER);
    const GLchar* vertShaderStr = vertShaderString.c_str();
    glShaderSource (vertShader, 1, &vertShaderStr, 0);
    glCompileShader (vertShader);
    GLint success;
    glGetShaderiv (vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      //{{{  error, exit
      char errMessage[512];
      glGetProgramInfoLog (vertShader, 512, NULL, errMessage);
      cLog::log (LOGERROR, fmt::format ("vertShader failed {}", errMessage));

      exit (EXIT_FAILURE);
      }
      //}}}

    // compile fragShader
    const GLuint fragShader = glCreateShader (GL_FRAGMENT_SHADER);
    const GLchar* fragShaderStr = fragShaderString.c_str();
    glShaderSource (fragShader, 1, &fragShaderStr, 0);
    glCompileShader (fragShader);
    glGetShaderiv (fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
      //{{{  error, exit
      char errMessage[512];
      glGetProgramInfoLog (fragShader, 512, NULL, errMessage);
      cLog::log (LOGERROR, fmt::format ("fragShader failed {}", errMessage));
      exit (EXIT_FAILURE);
      }
      //}}}

    // create shader program
    uint32_t id = glCreateProgram();
    glAttachShader (id, vertShader);
    glAttachShader (id, fragShader);
    glLinkProgram (id);
    glGetProgramiv (id, GL_LINK_STATUS, &success);
    if (!success) {
      //{{{  error, exit
      char errMessage[512];
      glGetProgramInfoLog (id, 512, NULL, errMessage);
      cLog::log (LOGERROR, fmt::format ("shaderProgram failed {} ",  errMessage));

      exit (EXIT_FAILURE);
      }
      //}}}

    glDeleteShader (vertShader);
    glDeleteShader (fragShader);

    return id;
    }
  //}}}
  //{{{
  class cOpenGlLayerShader : public cLayerShader {
  public:
    //{{{
    cOpenGlLayerShader() : cLayerShader() {

      #ifdef OPENGL_21
        //{{{
        const string kFragShader =
          "#version 120\n"

          "uniform sampler2D uSampler;"
          "uniform float uHue;"
          "uniform float uVal;"
          "uniform float uSat;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          //{{{
          "vec3 rgbToHsv (float r, float g, float b) {"
          "  float max_val = max(r, max(g, b));"
          "  float min_val = min(r, min(g, b));"
          "  float h;" // hue in degrees

          "  if (max_val == min_val) {" // Simple default case. Do NOT increase saturation if this is the case!
          "    h = 0.0; }"
          "  else if (max_val == r) {"
          "    h = 60.0 * (0.0 + (g - b) / (max_val - min_val)); }"
          "  else if (max_val == g) {"
          "    h = 60.0 * (2.0 + (b - r)/ (max_val - min_val)); }"
          "  else if (max_val == b) {"
          "    h = 60.0 * (4.0 + (r - g) / (max_val - min_val)); }"
          "  if (h < 0.0) {"
          "    h += 360.0; }"

          "  float s = max_val == 0.0 ? 0.0 : (max_val - min_val) / max_val;"
          "  float v = max_val;"
          "  return vec3 (h, s, v);"
          "  }"
          //}}}
          //{{{
          "vec3 hsvToRgb (float h, float s, float v) {"
          "  float r, g, b;"
          "  float c = v * s;"
          "  float h_ = mod(h / 60.0, 6);" // For convenience, change to multiples of 60
          "  float x = c * (1.0 - abs(mod(h_, 2) - 1));"
          "  float r_, g_, b_;"

          "  if (0.0 <= h_ && h_ < 1.0) {"
          "    r_ = c, g_ = x, b_ = 0.0; }"
          "  else if (1.0 <= h_ && h_ < 2.0) {"
          "    r_ = x, g_ = c, b_ = 0.0; }"
          "  else if (2.0 <= h_ && h_ < 3.0) {"
          "    r_ = 0.0, g_ = c, b_ = x; }"
          "  else if (3.0 <= h_ && h_ < 4.0) {"
          "    r_ = 0.0, g_ = x, b_ = c; }"
          "  else if (4.0 <= h_ && h_ < 5.0) {"
          "    r_ = x, g_ = 0.0, b_ = c; }"
          "  else if (5.0 <= h_ && h_ < 6.0) {"
          "    r_ = c, g_ = 0.0, b_ = x; }"
          "  else {"
          "    r_ = 0.0, g_ = 0.0, b_ = 0.0; }"

          "  float m = v - c;"
          "  r = r_ + m;"
          "  g = g_ + m;"
          "  b = b_ + m;"

          "  return vec3 (r, g, b);"
          "  }"
          //}}}

          "void main() {"
          "  outColor = texture (uSampler, textureCoord);"

          "  if (uHue != 0.0 || uVal != 0.0 || uSat != 0.0) {"
          "    vec3 hsv = rgbToHsv (outColor.x, outColor.y, outColor.z);"
          "    hsv.x += uHue;"
          "    if ((outColor.x != outColor.y) || (outColor.y != outColor.z)) {"
                 // not grayscale
          "      hsv.y = uSat <= 0.0 ? "
          "      hsv.y * (1.0 + uSat) : hsv.y + (1.0 - hsv.y) * uSat;"
          "      }"
          "    hsv.z = uVal <= 0.0 ? hsv.z * (1.0 + uVal) : hsv.z + (1.0 - hsv.z) * uVal;"
          "    vec3 rgb = hsvToRgb (hsv.x, hsv.y, hsv.z);"
          "    outColor.xyz = rgb;"
          "    }"

          //"  if (uPreMultiply)"
          //"    outColor.xyz *= outColor.w;"
          "  }";
        //}}}
      #else
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D uSampler;"
          "uniform float uHue;"
          "uniform float uVal;"
          "uniform float uSat;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          //{{{
          "vec3 rgbToHsv (float r, float g, float b) {"
          "  float max_val = max(r, max(g, b));"
          "  float min_val = min(r, min(g, b));"
          "  float h;" // hue in degrees

          "  if (max_val == min_val) {" // Simple default case. Do NOT increase saturation if this is the case!
          "    h = 0.0; }"
          "  else if (max_val == r) {"
          "    h = 60.0 * (0.0 + (g - b) / (max_val - min_val)); }"
          "  else if (max_val == g) {"
          "    h = 60.0 * (2.0 + (b - r)/ (max_val - min_val)); }"
          "  else if (max_val == b) {"
          "    h = 60.0 * (4.0 + (r - g) / (max_val - min_val)); }"
          "  if (h < 0.0) {"
          "    h += 360.0; }"

          "  float s = max_val == 0.0 ? 0.0 : (max_val - min_val) / max_val;"
          "  float v = max_val;"
          "  return vec3 (h, s, v);"
          "  }"
          //}}}
          //{{{
          "vec3 hsvToRgb (float h, float s, float v) {"
          "  float r, g, b;"
          "  float c = v * s;"
          "  float h_ = mod(h / 60.0, 6);" // For convenience, change to multiples of 60
          "  float x = c * (1.0 - abs(mod(h_, 2) - 1));"
          "  float r_, g_, b_;"

          "  if (0.0 <= h_ && h_ < 1.0) {"
          "    r_ = c, g_ = x, b_ = 0.0; }"
          "  else if (1.0 <= h_ && h_ < 2.0) {"
          "    r_ = x, g_ = c, b_ = 0.0; }"
          "  else if (2.0 <= h_ && h_ < 3.0) {"
          "    r_ = 0.0, g_ = c, b_ = x; }"
          "  else if (3.0 <= h_ && h_ < 4.0) {"
          "    r_ = 0.0, g_ = x, b_ = c; }"
          "  else if (4.0 <= h_ && h_ < 5.0) {"
          "    r_ = x, g_ = 0.0, b_ = c; }"
          "  else if (5.0 <= h_ && h_ < 6.0) {"
          "    r_ = c, g_ = 0.0, b_ = x; }"
          "  else {"
          "    r_ = 0.0, g_ = 0.0, b_ = 0.0; }"

          "  float m = v - c;"
          "  r = r_ + m;"
          "  g = g_ + m;"
          "  b = b_ + m;"

          "  return vec3 (r, g, b);"
          "  }"
          //}}}

          "void main() {"
          "  outColor = texture (uSampler, textureCoord);"

          "  if (uHue != 0.0 || uVal != 0.0 || uSat != 0.0) {"
          "    vec3 hsv = rgbToHsv (outColor.x, outColor.y, outColor.z);"
          "    hsv.x += uHue;"
          "    if ((outColor.x != outColor.y) || (outColor.y != outColor.z)) {"
                 // not grayscale
          "      hsv.y = uSat <= 0.0 ? "
          "      hsv.y * (1.0 + uSat) : hsv.y + (1.0 - hsv.y) * uSat;"
          "      }"
          "    hsv.z = uVal <= 0.0 ? hsv.z * (1.0 + uVal) : hsv.z + (1.0 - hsv.z) * uVal;"
          "    vec3 rgb = hsvToRgb (hsv.x, hsv.y, hsv.z);"
          "    outColor.xyz = rgb;"
          "    }"

          //"  if (uPreMultiply)"
          //"    outColor.xyz *= outColor.w;"
          "  }";
      #endif

      mId = compileShader (kQuadVertShader, kFragShader);
      }
    //}}}
    //{{{
    virtual ~cOpenGlLayerShader() {
      glDeleteProgram (mId);
      }
    //}}}

    // sets
    //{{{
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

      glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
      glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
      }
    //}}}
    //{{{
    virtual void setHueSatVal (float hue, float sat, float val) final {
      glUniform1f (glGetUniformLocation (mId, "uHue"), hue);
      glUniform1f (glGetUniformLocation (mId, "uSat"), sat);
      glUniform1f (glGetUniformLocation (mId, "uVal"), val);
      }
    //}}}

    //{{{
    virtual void use() final {

      glUseProgram (mId);
      }
    //}}}
    };
  //}}}
  //{{{
  class cOpenGlPaintShader : public cPaintShader {
  public:
    //{{{
    cOpenGlPaintShader() : cPaintShader() {

      #ifdef OPENGL_21
        const string kFragShader =
          "#version 120\n"

          "uniform sampler2D uSampler;"
          "uniform vec2 uPos;"
          "uniform vec2 uPrevPos;"
          "uniform float uRadius;"
          "uniform vec4 uColor;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "float distToLine (vec2 v, vec2 w, vec2 p) {"
          "  float l2 = pow (distance(w, v), 2.);"
          "  if (l2 == 0.0)"
          "    return distance (p, v);"
          "  float t = clamp (dot (p - v, w - v) / l2, 0., 1.);"
          "  vec2 j = v + t * (w - v);"
          "  return distance (p, j);"
          "  }"

          "void main() {"
          "  float dist = distToLine (uPrevPos.xy, uPos.xy, textureCoord * textureSize (uSampler, 0)) - uRadius;"
          "  outColor = mix (uColor, texture (uSampler, textureCoord), clamp (dist, 0.0, 1.0));"
          "  }";
      #else
        const string kFragShader =
          "#version 330 core\n"

          "uniform sampler2D uSampler;"
          "uniform vec2 uPos;"
          "uniform vec2 uPrevPos;"
          "uniform float uRadius;"
          "uniform vec4 uColor;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "float distToLine (vec2 v, vec2 w, vec2 p) {"
          "  float l2 = pow (distance(w, v), 2.);"
          "  if (l2 == 0.0)"
          "    return distance (p, v);"
          "  float t = clamp (dot (p - v, w - v) / l2, 0., 1.);"
          "  vec2 j = v + t * (w - v);"
          "  return distance (p, j);"
          "  }"

          "void main() {"
          "  float dist = distToLine (uPrevPos.xy, uPos.xy, textureCoord * textureSize (uSampler, 0)) - uRadius;"
          "  outColor = mix (uColor, texture (uSampler, textureCoord), clamp (dist, 0.0, 1.0));"
          "  }";
      #endif

      mId = compileShader (kQuadVertShader, kFragShader);
      }
    //}}}
    //{{{
    virtual ~cOpenGlPaintShader() {
      glDeleteProgram (mId);
      }
    //}}}

    // sets
    //{{{
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
      glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
      }
    //}}}
    //{{{
    virtual void setStroke (cVec2 pos, cVec2 prevPos, float radius, const cColor& color) final {

      glUniform2fv (glGetUniformLocation (mId, "uPos"), 1, (float*)&pos);
      glUniform2fv (glGetUniformLocation (mId, "uPrevPos"), 1, (float*)&prevPos);
      glUniform1f (glGetUniformLocation (mId, "uRadius"), radius);
      glUniform4fv (glGetUniformLocation (mId, "uColor"), 1, (float*)&color);
      }
    //}}}

    //{{{
    virtual void use() final {

      glUseProgram (mId);
      }
    //}}}
    };
  //}}}

  //{{{
  class cOpenGlRgbaTexture : public cTexture {
  public:
    //{{{
    cOpenGlRgbaTexture (eTextureType textureType, const cPoint& size)
        : cTexture(textureType, size) {

      cLog::log (LOGINFO, fmt::format ("creating eRgba texture {}x{}", size.x, size.y));
      glGenTextures (1, &mTextureId);

      glBindTexture (GL_TEXTURE_2D, mTextureId);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glGenerateMipmap (GL_TEXTURE_2D);
      }
    //}}}
    //{{{
    virtual ~cOpenGlRgbaTexture() {
      glDeleteTextures (1, &mTextureId);
      }
    //}}}

    virtual unsigned getTextureId() const final { return mTextureId; }

    //{{{
    virtual void setPixels (uint8_t** pixels) final {
    // set textures using pixels in ffmpeg avFrame format

      glBindTexture (GL_TEXTURE_2D, mTextureId);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels[0]);
      }
    //}}}
    //{{{
    virtual void setSource() final {

      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, mTextureId);
      }
    //}}}

  private:
    uint32_t mTextureId = 0;
    };
  //}}}
  //{{{
  class cOpenGlNv12Texture : public cTexture {
  public:
    //{{{
    cOpenGlNv12Texture (eTextureType textureType, const cPoint& size)
        : cTexture(textureType, size) {

      //cLog::log (LOGINFO, fmt::format ("creating eNv12 texture {}x{}", size.x, size.y));

      glGenTextures (2, mTextureId.data());

      // y texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // uv texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
      #ifndef OPENGL2
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, size.x/2, size.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, nullptr);
      #endif
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
    //}}}
    //{{{
    virtual ~cOpenGlNv12Texture() {
      //glDeleteTextures (2, mTextureId.data());
      glDeleteTextures (1, &mTextureId[0]);
      glDeleteTextures (1, &mTextureId[1]);

      cLog::log (LOGINFO, fmt::format ("deleting eVv12 texture {}x{}", mSize.x, mSize.y));
      }
    //}}}

    virtual unsigned getTextureId() const final { return mTextureId[0]; }  // luma only

    //{{{
    virtual void setPixels (uint8_t** pixels) final {
    // set textures using pixels in ffmpeg avFrame format

      // y texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

      #ifndef OPENGL2
      // uv texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RG, mSize.x/2, mSize.y/2, 0, GL_RG, GL_UNSIGNED_BYTE, pixels[1]);
      #endif
      }
    //}}}
    //{{{
    virtual void setSource() final {

      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, mTextureId[0]);

      glActiveTexture (GL_TEXTURE1);
      glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
      }
    //}}}

  private:
    array <uint32_t,2> mTextureId;
    };
  //}}}
  //{{{
  class cOpenGlYuv420Texture : public cTexture {
  public:
    //{{{
    cOpenGlYuv420Texture (eTextureType textureType, const cPoint& size)
        : cTexture(textureType, size) {

      //cLog::log (LOGINFO, fmt::format ("creating eYuv420 texture {}x{}", size.x, size.y));

      glGenTextures (3, mTextureId.data());

      // y texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // u texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // v texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
    //}}}
    //{{{
    virtual ~cOpenGlYuv420Texture() {
      //glDeleteTextures (3, mTextureId.data());
      glDeleteTextures (1, &mTextureId[0]);
      glDeleteTextures (1, &mTextureId[1]);
      glDeleteTextures (1, &mTextureId[2]);

      cLog::log (LOGINFO, fmt::format ("deleting eYuv420 texture {}x{}", mSize.x, mSize.y));
      }
    //}}}

    virtual unsigned getTextureId() const final { return mTextureId[0]; }   // luma only

    //{{{
    virtual void setPixels (uint8_t** pixels) final {
    // set textures using pixels in ffmpeg avFrame format

      // y texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x, mSize.y, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[0]);

      // u texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[1]);

      // v texture
      glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RED, mSize.x/2, mSize.y/2, 0, GL_RED, GL_UNSIGNED_BYTE, pixels[2]);
      }
    //}}}
    //{{{
    virtual void setSource() final {

      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, mTextureId[0]);

      glActiveTexture (GL_TEXTURE1);
      glBindTexture (GL_TEXTURE_2D, mTextureId[1]);

      glActiveTexture (GL_TEXTURE2);
      glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
      }
    //}}}

  private:
    array <uint32_t,3> mTextureId;
    };
  //}}}

  //{{{
  class cOpenGlRgbaShader : public cTextureShader {
  public:
    //{{{
    cOpenGlRgbaShader() : cTextureShader() {

      #ifdef OPENGL_21
        const string kFragShader =
          "#version 120\n"

          "uniform sampler2D uSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
          "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
          //"  outColor /= outColor.w;"
          "  }";
      #else
        const string kFragShader =
          "#version 330 core\n"
          "uniform sampler2D uSampler;"

          "in vec2 textureCoord;"
          "out vec4 outColor;"

          "void main() {"
          "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
          //"  outColor /= outColor.w;"
          "  }";
      #endif

      mId = compileShader (kQuadVertShader, kFragShader);
      }
    //}}}
    //{{{
    virtual ~cOpenGlRgbaShader() {
      glDeleteProgram (mId);
      }
    //}}}

    //{{{
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
      glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
      }
    //}}}
    //{{{
    virtual void use() final {

      glUseProgram (mId);

      glUniform1i (glGetUniformLocation (mId, "uSampler"), 0);
      }
    //}}}
    };
  //}}}
  //{{{
  class cOpenGlNv12Shader : public cTextureShader {
  public:
    cOpenGlNv12Shader() : cTextureShader() {
      const string kFragShader =
        "#version 330 core\n"
        "uniform sampler2D ySampler;"
        "uniform sampler2D uvSampler;"

        "in vec2 textureCoord;"
        "out vec4 outColor;"

        "void main() {"
          "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
          "float u = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
          "float v = texture (uvSampler, vec2 (textureCoord.x, -textureCoord.y)).g - 0.5;"
          "y = (y - 0.0625) * 1.1643;"
          "outColor.r = y + (1.5958 * v);"
          "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
          "outColor.b = y + (2.017 * u);"
          "outColor.a = 1.0;"
          "}";

      mId = compileShader (kQuadVertShader, kFragShader);
      }

    //{{{
    virtual ~cOpenGlNv12Shader() {
      glDeleteProgram (mId);
      }
    //}}}

    //{{{
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

      glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
      glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
      }
    //}}}
    //{{{
    virtual void use() final {

      //cLog::log (LOGINFO, "video use");
      glUseProgram (mId);

      glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
      glUniform1i (glGetUniformLocation (mId, "uvSampler"), 1);
      }
    //}}}
    };
  //}}}
  //{{{
  class cOpenGlYuv420Shader : public cTextureShader {
  public:
    cOpenGlYuv420Shader() : cTextureShader() {
      const string kFragShader =
        "#version 330 core\n"
        "uniform sampler2D ySampler;"
        "uniform sampler2D uSampler;"
        "uniform sampler2D vSampler;"

        "in vec2 textureCoord;"
        "out vec4 outColor;"

        "void main() {"
          "float y = texture (ySampler, vec2 (textureCoord.x, -textureCoord.y)).r;"
          "float u = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
          "float v = texture (vSampler, vec2 (textureCoord.x, -textureCoord.y)).r - 0.5;"
          "y = (y - 0.0625) * 1.1643;"
          "outColor.r = y + (1.5958 * v);"
          "outColor.g = y - (0.39173 * u) - (0.81290 * v);"
          "outColor.b = y + (2.017 * u);"
          "outColor.a = 1.0;"
          "}";

      mId = compileShader (kQuadVertShader, kFragShader);
      }
    //{{{
    virtual ~cOpenGlYuv420Shader() {
      glDeleteProgram (mId);
      }
    //}}}

    //{{{
    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {

      glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
      glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
      }
    //}}}
    //{{{
    virtual void use() final {

      //cLog::log (LOGINFO, "video use");
      glUseProgram (mId);

      glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
      glUniform1i (glGetUniformLocation (mId, "uSampler"), 1);
      glUniform1i (glGetUniformLocation (mId, "vSampler"), 2);
      }
    //}}}
    };
  //}}}

  // versions
  int gGlVersion = 0;   // major.minor * 100
  int gGlslVersion = 0; // major.minor * 100

  #ifndef USE_IMPL
    //{{{  local implementation of backend impl
    bool gDrawBase = false;
    bool gClipOriginBottomLeft = true;

    // resources
    GLuint gFontTexture = 0;
    bool gFontLoaded = false;
    //{{{
    void createFontTexture() {
    // load font texture atlas as RGBA 32-bit (75% of the memory is wasted, but default font is so small)
    // because it is more likely to be compatible with user's existing shaders
    // If your ImTextureId represent a higher-level concept than just a GL texture id,
    // consider calling GetTexDataAsAlpha8() instead to save on GPU memory

      uint8_t* pixels;
      int32_t width;
      int32_t height;
      ImGui::GetIO().Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

      // create texture
      glGenTextures (1, &gFontTexture);
      glBindTexture (GL_TEXTURE_2D, gFontTexture);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      // pixels to texture
      glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

      // set font textureId
      ImGui::GetIO().Fonts->TexID = (ImTextureID)(intptr_t)gFontTexture;
      }
    //}}}

    //{{{
    class cDrawListShader : public cShader {
    public:
      //{{{
      cDrawListShader (uint32_t glslVersion) : cShader() {

        if (glslVersion == 120) {
          //{{{
          const string kVertShader120 =
            "#version 100\n"

            "uniform mat4 ProjMtx;"

            "attribute vec2 Position;"
            "attribute vec2 UV;"
            "attribute vec4 Color;"

            "varying vec2 Frag_UV;"
            "varying vec4 Frag_Color;"

            "void main() {"
            "  Frag_UV = UV;"
            "  Frag_Color = Color;"
            "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
            "  }";
          //}}}
          //{{{
          const string kFragShader120 =
            "#version 100\n"

            "#ifdef GL_ES\n"
            "  precision mediump float;"
            "#endif\n"

            "uniform sampler2D Texture;"

            "varying vec2 Frag_UV;"
            "varying vec4 Frag_Color;"

            "void main() {"
            "  gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);"
            "  }";
          //}}}
          mId = compileShader (kVertShader120, kFragShader120);
          }
        else if (glslVersion == 300) {
          //{{{
          const string kVertShader300es =
            "#version 300 es\n"

            "precision mediump float;"

            "layout (location = 0) in vec2 Position;"
            "layout (location = 1) in vec2 UV;"
            "layout (location = 2) in vec4 Color;"

            "uniform mat4 ProjMtx;"

            "out vec2 Frag_UV;"
            "out vec4 Frag_Color;"

            "void main() {"
            "  Frag_UV = UV;"
            "  Frag_Color = Color;"
            "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
            "  }";
          //}}}
          //{{{
          const string kFragShader300es =
            "#version 300 es\n"

            "precision mediump float;"

            "uniform sampler2D Texture;"

            "in vec2 Frag_UV;"
            "in vec4 Frag_Color;"

            "layout (location = 0) out vec4 Out_Color;"

            "void main() {"
            "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
            "  }";
          //}}}
          mId = compileShader (kVertShader300es, kFragShader300es);
          }
        else if (glslVersion >= 410) {
          //{{{
          const string kVertShader410core =
            "#version 410 core\n"

            "layout (location = 0) in vec2 Position;"
            "layout (location = 1) in vec2 UV;"
            "layout (location = 2) in vec4 Color;"

            "uniform mat4 ProjMtx;"

            "out vec2 Frag_UV;"
            "out vec4 Frag_Color;"

            "void main() {"
            "  Frag_UV = UV;"
            "  Frag_Color = Color;"
            "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
            "  }";
          //}}}
          //{{{
          const string kFragShader410core =
            "#version 410 core\n"

            "in vec2 Frag_UV;"
            "in vec4 Frag_Color;"

            "uniform sampler2D Texture;"

            "layout (location = 0) out vec4 Out_Color;"

            "void main() {"
            "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
            "  }";
          //}}}
          mId = compileShader (kVertShader410core, kFragShader410core);
          }
        else {
          //{{{
          const string kVertShader130 =
            "#version 130\n"

            "uniform mat4 ProjMtx;"

            "in vec2 Position;"
            "in vec2 UV;"
            "in vec4 Color;"

            "out vec2 Frag_UV;"
            "out vec4 Frag_Color;"

            "void main() {"
            "  Frag_UV = UV;"
            "  Frag_Color = Color;"
            "  gl_Position = ProjMtx * vec4(Position.xy,0,1);"
            "  }";
          //}}}
          //{{{
          const string kFragShader130 =
            "#version 130\n"

            "uniform sampler2D Texture;"

            "in vec2 Frag_UV;"
            "in vec4 Frag_Color;"

            "out vec4 Out_Color;"

            "void main() {"
            "  Out_Color = Frag_Color * texture(Texture, Frag_UV.st);"
            "  }";
          //}}}
          mId = compileShader (kVertShader130, kFragShader130);
          }

        // store uniform locations
        mAttribLocationTexture = glGetUniformLocation (getId(), "Texture");
        mAttribLocationProjMtx = glGetUniformLocation (getId(), "ProjMtx");

        mAttribLocationVtxPos = glGetAttribLocation (getId(), "Position");
        mAttribLocationVtxUV = glGetAttribLocation (getId(), "UV");
        mAttribLocationVtxColor = glGetAttribLocation (getId(), "Color");
        }
      //}}}
      virtual ~cDrawListShader() = default;

      // gets
      int32_t getAttribLocationVtxPos() { return mAttribLocationVtxPos; }
      int32_t getAttribLocationVtxUV() { return mAttribLocationVtxUV; }
      int32_t getAttribLocationVtxColor() { return mAttribLocationVtxColor; }

      // sets
      //{{{
      void setMatrix (const cMat4x4& matrix) {
        glUniformMatrix4fv (mAttribLocationProjMtx, 1, GL_FALSE, (float*)&matrix);
        }
      //}}}

      //{{{
      void use() final {

        glUseProgram (mId);
        }
      //}}}

    private:
      int32_t mAttribLocationTexture = 0;
      int32_t mAttribLocationProjMtx = 0;

      int32_t mAttribLocationVtxPos = 0;
      int32_t mAttribLocationVtxUV = 0;
      int32_t mAttribLocationVtxColor = 0;
      };
    //}}}
    cDrawListShader* gDrawListShader;

    GLuint gVboHandle = 0;
    GLuint gElementsHandle = 0;
    //{{{
    void setRenderState (ImDrawData* drawData, int width, int height, GLuint vertexArrayObject) {

      // disables
      glDisable (GL_CULL_FACE);
      glDisable (GL_DEPTH_TEST);

      #ifndef OPENGL2
        glDisable(GL_PRIMITIVE_RESTART);
      #endif

      // enables
      glEnable (GL_SCISSOR_TEST);
      glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

      // set blend = lerp
      glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glBlendEquation (GL_FUNC_ADD);
      glEnable (GL_BLEND);

      gDrawListShader->use();
      glViewport (0, 0, (GLsizei)width, (GLsizei)height);

      // set orthoProject matrix
      float left = drawData->DisplayPos.x;
      float right = drawData->DisplayPos.x + drawData->DisplaySize.x;
      float top = drawData->DisplayPos.y;;
      float bottom = drawData->DisplayPos.y;;
      if (gClipOriginBottomLeft) // bottom left origin ortho project
        bottom += drawData->DisplaySize.y;
      else // top left origin ortho project
        top += drawData->DisplaySize.y;
      gDrawListShader->setMatrix (cMat4x4(left, right, bottom, top, -1.f, 1.f));

      // bind vertex/index buffers and setup attributes for ImDrawVert
      glBindVertexArray (vertexArrayObject);

      glBindBuffer (GL_ARRAY_BUFFER, gVboHandle);
      glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, gElementsHandle);

      glEnableVertexAttribArray (gDrawListShader->getAttribLocationVtxPos());
      glVertexAttribPointer (gDrawListShader->getAttribLocationVtxPos(), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                             (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));

      glEnableVertexAttribArray (gDrawListShader->getAttribLocationVtxUV());
      glVertexAttribPointer (gDrawListShader->getAttribLocationVtxUV(), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                             (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));

      glEnableVertexAttribArray (gDrawListShader->getAttribLocationVtxColor());
      glVertexAttribPointer (gDrawListShader->getAttribLocationVtxColor(), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                             (GLvoid*)IM_OFFSETOF(ImDrawVert, col));
      }
    //}}}

    //{{{
    void renderDrawData (ImDrawData* drawData) {

      // avoid rendering when minimized
      int width = static_cast<int>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
      int height = static_cast<int>(drawData->DisplaySize.y * drawData->FramebufferScale.y);

      if ((width > 0) && (height > 0)) {
        // temp VAO
        GLuint vertexArrayObject;
        glGenVertexArrays (1, &vertexArrayObject);

        setRenderState (drawData, width, height, vertexArrayObject);

        // project scissor/clipping rectangles into framebuffer space
        ImVec2 clipOff = drawData->DisplayPos; // (0,0) unless using multi-viewports

        glActiveTexture (GL_TEXTURE0);

        // render command lists
        for (int n = 0; n < drawData->CmdListsCount; n++) {
          const ImDrawList* cmdList = drawData->CmdLists[n];

          // upload vertex/index buffers
          glBufferData (GL_ARRAY_BUFFER,
                        (GLsizeiptr)cmdList->VtxBuffer.Size * static_cast<int>(sizeof(ImDrawVert)),
                        (const GLvoid*)cmdList->VtxBuffer.Data, GL_STREAM_DRAW);
          glBufferData (GL_ELEMENT_ARRAY_BUFFER,
                        (GLsizeiptr)cmdList->IdxBuffer.Size * static_cast<int>(sizeof(ImDrawIdx)),
                        (const GLvoid*)cmdList->IdxBuffer.Data, GL_STREAM_DRAW);

          for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
            // project scissor/clipping rect into framebuffer space
            cRect clipRect (static_cast<int>(pcmd->ClipRect.x - clipOff.x),
                            static_cast<int>(pcmd->ClipRect.y - clipOff.y),
                            static_cast<int>(pcmd->ClipRect.z - clipOff.x),
                            static_cast<int>(pcmd->ClipRect.w - clipOff.y));

            // test for on screen
            if ((clipRect.right >= 0) && (clipRect.bottom >= 0) && (clipRect.left < width) && (clipRect.top < height)) {
              // scissor clip
              glScissor (clipRect.left, height - clipRect.bottom, clipRect.getWidth(), clipRect.getHeight());

              // bind
              glBindTexture (GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);

              // draw
             #ifndef OPENGL2
             if (gDrawBase)
                glDrawElementsBaseVertex (GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                          sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                          (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
                                          (GLint)pcmd->VtxOffset);
             else
             #endif
               glDrawElements (GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
              }
            }
          }

        // delete temp VAO
        glDeleteVertexArrays (1, &vertexArrayObject);
        }
      }
    //}}}
    //{{{
    void renderWindow (ImGuiViewport* viewport, void*) {

      if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear)) {
        ImVec4 clear_color = ImVec4(0.f,0.f,0.f, 1.f);
        glClearColor (clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear (GL_COLOR_BUFFER_BIT);
        }

      renderDrawData (viewport->DrawData);
      }
    //}}}
    //}}}
  #endif
  }

// cOpenGlGraphics class header
//{{{
class cOpenGlGraphics : public cGraphics {
public:
  void shutdown() final;

  // create resources
  virtual cQuad* createQuad (const cPoint& size) final;
  virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final;

  virtual cFrameBuffer* createFrameBuffer() final;
  virtual cFrameBuffer* createFrameBuffer (const cPoint& size, cFrameBuffer::eFormat format) final;
  virtual cFrameBuffer* createFrameBuffer (uint8_t* pixels, const cPoint& size, cFrameBuffer::eFormat format) final;

  virtual cLayerShader* createLayerShader() final;
  virtual cPaintShader* createPaintShader() final;

  virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final;
  virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final;

  virtual void background (const cPoint& size) final;

  // actions
  virtual void newFrame() final;
  virtual void drawUI (const cPoint& windowSize) final;
  virtual void windowResize (int width, int height) final;

protected:
  virtual bool init (cPlatform& platform) final;

private:
  // static register
  static cGraphics* create (const std::string& className);
  inline static const bool mRegistered = registerClass ("opengl", &create);
  };
//}}}

// public:
//{{{
void cOpenGlGraphics::shutdown() {

  #ifdef USE_IMPL
    ImGui_ImplOpenGL3_Shutdown();
  #else
    ImGui::DestroyPlatformWindows();

    // destroy vertetx array
    if (gVboHandle)
      glDeleteBuffers (1, &gVboHandle);
    if (gElementsHandle)
      glDeleteBuffers (1, &gElementsHandle);

    // destroy shader
    delete gDrawListShader;

    // destroy font texture
    if (gFontTexture) {
      glDeleteTextures (1, &gFontTexture);
      ImGui::GetIO().Fonts->TexID = 0;
      }
  #endif
  }
//}}}

// - resource creates
//{{{
cQuad* cOpenGlGraphics::createQuad (const cPoint& size) {
  return new cOpenGlQuad (size);
  }
//}}}
//{{{
cQuad* cOpenGlGraphics::createQuad (const cPoint& size, const cRect& rect) {
  return new cOpenGlQuad (size, rect);
  }
//}}}

//{{{
cFrameBuffer* cOpenGlGraphics::createFrameBuffer() {
  return new cOpenGlFrameBuffer();
  }
//}}}
//{{{
cFrameBuffer* cOpenGlGraphics::createFrameBuffer (const cPoint& size, cFrameBuffer::eFormat format) {
  return new cOpenGlFrameBuffer (size, format);
  }
//}}}
//{{{
cFrameBuffer* cOpenGlGraphics::createFrameBuffer (uint8_t* pixels, const cPoint& size, cFrameBuffer::eFormat format) {
  return new cOpenGlFrameBuffer (pixels, size, format);
  }
//}}}

//{{{
cLayerShader* cOpenGlGraphics::createLayerShader() {
  return new cOpenGlLayerShader();
  }
//}}}
//{{{
cPaintShader* cOpenGlGraphics::createPaintShader() {
  return new cOpenGlPaintShader();
  }
//}}}

//{{{
cTexture* cOpenGlGraphics::createTexture (cTexture::eTextureType textureType, const cPoint& size) {
// factory create

  switch (textureType) {
    case cTexture::eRgba:   return new cOpenGlRgbaTexture (textureType, size);
    case cTexture::eNv12:   return new cOpenGlNv12Texture (textureType, size);
    case cTexture::eYuv420: return new cOpenGlYuv420Texture (textureType, size);
    default : return nullptr;
    }
  }
//}}}
//{{{
cTextureShader* cOpenGlGraphics::createTextureShader (cTexture::eTextureType textureType) {
// factory create

  switch (textureType) {
    case cTexture::eRgba:   return new cOpenGlRgbaShader();
    case cTexture::eYuv420: return new cOpenGlYuv420Shader();
    case cTexture::eNv12:   return new cOpenGlNv12Shader();
    default: return nullptr;
    }
  }
//}}}

//{{{
void cOpenGlGraphics::background (const cPoint& size) {

  glViewport (0, 0, size.x, size.y);

  // blend
  uint32_t modeRGB = GL_FUNC_ADD;
  uint32_t modeAlpha = GL_FUNC_ADD;
  glBlendEquationSeparate (modeRGB, modeAlpha);

  uint32_t srcRgb = GL_SRC_ALPHA;
  uint32_t dstRGB = GL_ONE_MINUS_SRC_ALPHA;
  uint32_t srcAlpha = GL_ONE;
  uint32_t dstAlpha = GL_ONE_MINUS_SRC_ALPHA;
  glBlendFuncSeparate (srcRgb, dstRGB, srcAlpha, dstAlpha);

  glEnable (GL_BLEND);

  // disables
  glDisable (GL_SCISSOR_TEST);
  glDisable (GL_CULL_FACE);
  glDisable (GL_DEPTH_TEST);

  // clear
  glClearColor (0.f,0.f,0.f, 0.f);
  glClear (GL_COLOR_BUFFER_BIT);
  }
//}}}

//{{{
void cOpenGlGraphics::windowResize (int width, int height) {
  cLog::log (LOGINFO, fmt::format ("cOpenGlGraphics windowResize {} {}", width, height));
  }
//}}}
//{{{
void cOpenGlGraphics::newFrame() {

  #ifdef USE_IMPL
    ImGui_ImplOpenGL3_NewFrame();
  #else
    if (!gFontLoaded) {
      createFontTexture();
      gFontLoaded = true;
      }
  #endif

  ImGui::NewFrame();
  }
//}}}
//{{{
void cOpenGlGraphics::drawUI (const cPoint& windowSize) {

  (void)windowSize;
  ImGui::Render();

  #ifdef USE_IMPL
    glViewport (0, 0, windowSize.x, windowSize.y);
    ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData());
  #else
    renderDrawData (ImGui::GetDrawData());
  #endif
  }
//}}}

// protected:
//{{{
bool cOpenGlGraphics::init (cPlatform& platform) {
// anonymous device pointers unused

  (void)platform;

  // get openGL version
  string glVersionString = (const char*)glGetString (GL_VERSION);
  #if defined(IMGUI_IMPL_OPENGL_ES2)
    gGlVersion = 200; // GLES 2
  #else
    GLint glMajor = 0;
    //glGetIntegerv (GL_MAJOR_VERSION, &glMajor);
    GLint glMinor = 0;
    //glGetIntegerv (GL_MINOR_VERSION, &glMinor);
    //if ((glMajor == 0) && (glMinor == 0))
    // Query GL_VERSION string desktop GL 2.x
    sscanf (glVersionString.c_str(), "%d.%d", &glMajor, &glMinor);
    gGlVersion = ((glMajor * 10) + glMinor) * 10;
  #endif
  cLog::log (LOGINFO, fmt::format ("OpenGL {} - {}", glVersionString, gGlVersion));


  #ifndef USE_IMPL
    // set imGui backend capabilities flags
    ImGui::GetIO().BackendRendererName = "openGL3";
    //{{{  vertexOffset
    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
      if (gGlVersion >= 320) {
        gDrawBase = true;
        ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
        cLog::log (LOGINFO, "- hasVtxOffset");
        }
    #endif
    //}}}
    //{{{  clipOrigin
    #if defined (GL_CLIP_ORIGIN)
      GLenum currentClipOrigin = 0;
      glGetIntegerv (GL_CLIP_ORIGIN, (GLint*)&currentClipOrigin);
      if (currentClipOrigin == GL_UPPER_LEFT)
        gClipOriginBottomLeft = false;
    #endif
    //}}}

    // enable ImGui viewports
    ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasViewports;

    // set ImGui renderWindow
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      ImGui::GetPlatformIO().Renderer_RenderWindow = renderWindow;
  #endif

  // get GLSL version
  string glslVersionString = (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION);
  int glslMajor = 0;
  int glslMinor = 0;
  sscanf (glslVersionString.c_str(), "%d.%d", &glslMajor, &glslMinor);
  gGlslVersion = (glslMajor * 100) + glslMinor;
  cLog::log (LOGINFO, fmt::format ("GLSL {} - {}", glslVersionString, gGlslVersion));

  #ifdef USE_IMPL
    ImGui_ImplOpenGL3_Init (glslVersionString.c_str());
  #else
    cLog::log (LOGINFO, fmt::format ("Renderer {}", glGetString (GL_RENDERER)));
    cLog::log (LOGINFO, fmt::format ("- Vendor {}", glGetString (GL_VENDOR)));

    cLog::log (LOGINFO, fmt::format ("imGui {} - {}", ImGui::GetVersion(), IMGUI_VERSION_NUM));
    cLog::log (LOGINFO, "- hasViewports");

    // create vertex array buffers
    glGenBuffers (1, &gVboHandle);
    glGenBuffers (1, &gElementsHandle);

    // create shader
    gDrawListShader = new cDrawListShader (gGlslVersion);
  #endif

  return true;
  }
//}}}

// private:
//{{{
cGraphics* cOpenGlGraphics::create (const string& className) {

  (void)className;
  return new cOpenGlGraphics();
  }
//}}}

// cGLES3Gaphics.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <array>

#include "cGraphics.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_opengl3.h"

#include "../common/cLog.h"
#include "fmt/format.h"
//}}}

class cGLES3Graphics : public cGraphics {
public:
  cGLES3Graphics (const std::string& glslVersion) : cGraphics(), mGlslVersion (glslVersion) {}
  //{{{
  virtual ~cGLES3Graphics() {
    ImGui_ImplOpenGL3_Shutdown();
    }
  //}}}

  // imgui
  //{{{
  virtual bool init() final {

    // report OpenGL versions
    cLog::log (LOGINFO, (const char*)glGetString (GL_VERSION));
    cLog::log (LOGINFO, (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION));
    cLog::log (LOGINFO, (const char*)glGetString (GL_RENDERER));
    cLog::log (LOGINFO, (const char*)glGetString (GL_VENDOR));

    return ImGui_ImplOpenGL3_Init (mGlslVersion.c_str());
    }
  //}}}
  virtual void newFrame() final { ImGui_ImplOpenGL3_NewFrame(); }
  //{{{
  virtual void clear (const cPoint& size) final {

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
  virtual void renderDrawData() final { ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData()); }

  // creates
  virtual cQuad* createQuad (const cPoint& size) final { return new cOpenGLES3Quad (size); }
  virtual cQuad* createQuad (const cPoint& size, const cRect& rect) final { return new cOpenGLES3Quad (size, rect); }

  virtual cTarget* createTarget() final { return new cOpenGLES3Target(); }
  virtual cTarget* createTarget (const cPoint& size, cTarget::eFormat format) final {
    return new cOpenGLES3Target (size, format); }
  virtual cTarget* createTarget (uint8_t* pixels, const cPoint& size, cTarget::eFormat format) final {
    return new cOpenGLES3Target (pixels, size, format); }

  //{{{
  virtual cTexture* createTexture (cTexture::eTextureType textureType, const cPoint& size) final {

    //cLog::log (LOGINFO, fmt::format ("cGLES3Gaphics::createTexture {} {}x{}",
    //                                 (int)textureType, size.x, size.y));
    switch (textureType) {
      case cTexture::eRgba:   return new cOpenGLES3RgbaTexture (textureType, size);
      case cTexture::eYuv420: return new cOpenGLES3Yuv420Texture (textureType, size);
      default : return nullptr;
      }
    }
  //}}}
  //{{{
  virtual cTextureShader* createTextureShader (cTexture::eTextureType textureType) final {
    switch (textureType) {
      case cTexture::eRgba:   return new cOpenGLES3RgbaShader();
      case cTexture::eYuv420: return new cOpenGLES3Yuv420Shader();
      default: return nullptr;
      }
    }
  //}}}

private:
  //{{{
  inline static const std::string kQuadVertShader =

    "#version 300 es\n"
    "precision highp float;"

    "uniform mat4 uModel;"
    "uniform mat4 uProject;"

    "layout (location = 0) in vec2 inPos;"
    "layout (location = 1) in vec2 inTextureCoord;"

    "out vec2 textureCoord;"

    "void main() {"
    "  textureCoord = inTextureCoord;"
    "  gl_Position = uProject * uModel * vec4 (inPos, 0.0, 1.0);"
    "  }";
  //}}}

  //{{{
  class cOpenGLES3Quad : public cQuad {
  public:
    //{{{
    cOpenGLES3Quad (const cPoint& size) : cQuad(size) {

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
    cOpenGLES3Quad (const cPoint& size, const cRect& rect) : cQuad(size) {

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
    virtual ~cOpenGLES3Quad() {

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
    inline static const uint8_t kIndices[mNumIndices] = {
      0, 1, 2, // 0   0-3
      0, 3, 1  // |\   \|
      };       // 2-1   1

    uint32_t mVertexArrayObject = 0;
    uint32_t mVertexBufferObject = 0;
    uint32_t mElementArrayBufferObject = 0;
    };
  //}}}
  //{{{
  class cOpenGLES3Target : public cTarget {
  public:
    //{{{
    cOpenGLES3Target() : cTarget ({0,0}) {
    // window Target

      mImageFormat = GL_RGBA;
      mInternalFormat = GL_RGBA;
      }
    //}}}
    //{{{
    cOpenGLES3Target (const cPoint& size, eFormat format) : cTarget(size) {

      mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
      mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

      // create empty Target object
      glGenFramebuffers (1, &mFrameBufferObject);
      glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

      // create and add texture to Target object
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
    cOpenGLES3Target (uint8_t* pixels, const cPoint& size, eFormat format) : cTarget(size) {

      mImageFormat = format == eRGBA ? GL_RGBA : GL_RGB;
      mInternalFormat = format == eRGBA ? GL_RGBA : GL_RGB;

      // create Target object from pixels
      glGenFramebuffers (1, &mFrameBufferObject);
      glBindFramebuffer (GL_FRAMEBUFFER, mFrameBufferObject);

      // create and add texture to Target object
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
    virtual ~cOpenGLES3Target() {

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
        mPixels = static_cast<uint8_t*>(malloc (getNumPixelBytes()));
        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        #if defined(GL3)
          glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
        #endif
        }

      else if (!mDirtyPixelsRect.isEmpty()) {
        // no openGL glGetTexSubImage, so dirtyPixelsRect not really used, is this correct ???
        glBindTexture (GL_TEXTURE_2D, mColorTextureId);
        #if defined(GL3)
          glGetTexImage (GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)mPixels);
        #endif
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
        cLog::log (LOGERROR, "unimplmented setSize of non screen Target");
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
        cLog::log (LOGERROR, "windowTarget cannot be src");
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
    void blit (cTarget& src, const cPoint& srcPoint, const cRect& dstRect) final {

      glBindFramebuffer (GL_READ_FRAMEBUFFER, src.getId());
      glBindFramebuffer (GL_DRAW_FRAMEBUFFER, mFrameBufferObject);
      glBlitFramebuffer (srcPoint.x, srcPoint.y, srcPoint.x + dstRect.getWidth(), srcPoint.y + dstRect.getHeight(),
                         dstRect.left, dstRect.top, dstRect.right, dstRect.bottom,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);

      // texture changed, add to dirtyPixelsRect
      mDirtyPixelsRect += dstRect;
      }
    //}}}

    //{{{
    bool checkStatus() final {

      GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);

      switch (status) {
        case GL_FRAMEBUFFER_COMPLETE:
          return true;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
          cLog::log (LOGERROR, "Target incomplete: Attachment is NOT complete"); return false;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
          cLog::log (LOGERROR, "Target incomplete: No image is attached to FBO"); return false;
        case GL_FRAMEBUFFER_UNSUPPORTED:
          cLog::log (LOGERROR, "Target incomplete: Unsupported by FBO implementation"); return false;
        default:
          cLog::log (LOGERROR, "Target incomplete: Unknown error"); return false;
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

      cLog::log (LOGINFO, fmt::format ("Target maxColorAttach {} masSamples {}", colorBufferCount, multiSampleCount));

      //  print info of the colorbuffer attachable image
      GLint objectType;
      GLint objectId;
      for (int i = 0; i < colorBufferCount; ++i) {
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                               GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
        if (objectType != GL_NONE) {
          glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i,
                                                 GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &objectId);
          std::string formatName;
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
      glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                             GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &objectType);
      if (objectType != GL_NONE) {
        glGetFramebufferAttachmentParameteriv (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
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
    static std::string getInternalFormat (uint32_t formatNum) {

      std::string formatName;

      switch (formatNum) {
        case GL_DEPTH_COMPONENT:   formatName = "GL_DEPTH_COMPONENT"; break;   // 0x1902
        case GL_ALPHA:             formatName = "GL_ALPHA"; break;             // 0x1906
        case GL_RGB:               formatName = "GL_RGB"; break;               // 0x1907
        case GL_RGB8:              formatName = "GL_RGB8"; break;              // 0x8051
        case GL_RGBA4:             formatName = "GL_RGBA4"; break;             // 0x8056
        case GL_RGB5_A1:           formatName = "GL_RGB5_A1"; break;           // 0x8057
        case GL_RGBA8:             formatName = "GL_RGBA8"; break;             // 0x8058
        case GL_RGB10_A2:          formatName = "GL_RGB10_A2"; break;          // 0x8059
        case GL_DEPTH_COMPONENT16: formatName = "GL_DEPTH_COMPONENT16"; break; // 0x81A5
        case GL_DEPTH_COMPONENT24: formatName = "GL_DEPTH_COMPONENT24"; break; // 0x81A6
        case GL_DEPTH_STENCIL:     formatName = "GL_DEPTH_STENCIL"; break;     // 0x84F9
        case GL_DEPTH24_STENCIL8:  formatName = "GL_DEPTH24_STENCIL8"; break;  // 0x88F0

      #if defined(GL3)
        case GL_STENCIL_INDEX:     formatName = "GL_STENCIL_INDEX"; break;     // 0x1901
        case GL_RGBA:              formatName = "GL_RGBA"; break;              // 0x1908
        case GL_R3_G3_B2:          formatName = "GL_R3_G3_B2"; break;          // 0x2A10
        case GL_RGB4:              formatName = "GL_RGB4"; break;              // 0x804F
        case GL_RGB5:              formatName = "GL_RGB5"; break;              // 0x8050
        case GL_RGB10:             formatName = "GL_RGB10"; break;             // 0x8052
        case GL_RGB12:             formatName = "GL_RGB12"; break;             // 0x8053
        case GL_RGB16:             formatName = "GL_RGB16"; break;             // 0x8054
        case GL_RGBA2:             formatName = "GL_RGBA2"; break;             // 0x8055
        case GL_RGBA12:            formatName = "GL_RGBA12"; break;            // 0x805A
        case GL_RGBA16:            formatName = "GL_RGBA16"; break;            // 0x805B
        case GL_DEPTH_COMPONENT32: formatName = "GL_DEPTH_COMPONENT32"; break; // 0x81A7
      #endif
        //case GL_LUMINANCE:         formatName = "GL_LUMINANCE"; break;         // 0x1909
        //case GL_LUMINANCE_ALPHA:   formatName = "GL_LUMINANCE_ALPHA"; break;   // 0x190A
        //case GL_RGBA32F:           formatName = "GL_RGBA32F"; break;           // 0x8814
        //case GL_RGB32F:            formatName = "GL_RGB32F"; break;            // 0x8815
        //case GL_RGBA16F:           formatName = "GL_RGBA16F"; break;           // 0x881A
        //case GL_RGB16F:            formatName = "GL_RGB16F"; break;            // 0x881B
        default: formatName = fmt::format ("Unknown Format {}", formatNum); break;
        }

      return formatName;
      }
    //}}}
    //{{{
    static std::string getTextureParameters (uint32_t id) {

      if (glIsTexture(id) == GL_FALSE)
        return "Not texture object";

      #if defined(GL3)
        glBindTexture (GL_TEXTURE_2D, id);
        int width;
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);            // get texture width
        int height;
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);          // get texture height
        int formatNum;
        glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &formatNum); // get texture internal format
        return fmt::format (" {} {} {}", width, height, getInternalFormat (formatNum));
      #else
        return "unknown texture";
      #endif

      }
    //}}}
    //{{{
    static std::string getRenderbufferParameters (uint32_t id) {

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

  //{{{
  class cOpenGLES3RgbaTexture : public cTexture {
  public:
    cOpenGLES3RgbaTexture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {

      //cLog::log (LOGINFO, fmt::format ("creating eRgba texture {}x{}", size.x, size.y));
      glGenTextures (1, &mTextureId);

      glBindTexture (GL_TEXTURE_2D, mTextureId);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glGenerateMipmap (GL_TEXTURE_2D);
      }

    virtual ~cOpenGLES3RgbaTexture() { glDeleteTextures (1, &mTextureId); }

    virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId; }

    virtual void setPixels (uint8_t** pixels, int* strides) final {
    // set textures using pixels in ffmpeg avFrame format
      (void)strides;

      glBindTexture (GL_TEXTURE_2D, mTextureId);
      glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, mSize.x, mSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels[0]);
      }

    virtual void setSource() final {
      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, mTextureId);
      }

  private:
    GLuint mTextureId = 0;
    };
  //}}}
  //{{{
  class cOpenGLES3Yuv420Texture : public cTexture {
  public:
    //{{{
    cOpenGLES3Yuv420Texture (eTextureType textureType, const cPoint& size) : cTexture(textureType, size) {

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
    virtual ~cOpenGLES3Yuv420Texture() {
      //glDeleteTextures (3, mTextureId.data());
      glDeleteTextures (1, &mTextureId[0]);
      glDeleteTextures (1, &mTextureId[1]);
      glDeleteTextures (1, &mTextureId[2]);

      cLog::log (LOGINFO, fmt::format ("deleting eYuv420 texture {}x{}", mSize.x, mSize.y));
      }
    //}}}

    virtual void* getTextureId() final { return (void*)(intptr_t)mTextureId[0]; }   // luma only

    virtual void setPixels (uint8_t** pixels, int* strides) final {
    // set textures using pixels in ffmpeg avFrame format
      (void)strides;

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

    virtual void setSource() final {
      glActiveTexture (GL_TEXTURE0);
      glBindTexture (GL_TEXTURE_2D, mTextureId[0]);
      glActiveTexture (GL_TEXTURE1);
      glBindTexture (GL_TEXTURE_2D, mTextureId[1]);
      glActiveTexture (GL_TEXTURE2);
      glBindTexture (GL_TEXTURE_2D, mTextureId[2]);
      }

  private:
    std::array <GLuint,3> mTextureId;  // Y,U,V 4:2:0
    };
  //}}}

  //{{{
  class cOpenGLES3RgbaShader : public cTextureShader {
  public:
    cOpenGLES3RgbaShader() : cTextureShader() {
      const std::string kFragShader =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform sampler2D uSampler;"
        "in vec2 textureCoord;"
        "out vec4 outColor;"
        "void main() {"
        "  outColor = texture (uSampler, vec2 (textureCoord.x, -textureCoord.y));"
        "  }";

      mId = compileShader (kQuadVertShader, kFragShader);
      }

    virtual ~cOpenGLES3RgbaShader() { glDeleteProgram (mId); }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
      glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
      }

    virtual void use() final {
      glUseProgram (mId);
      glUniform1i (glGetUniformLocation (mId, "uSampler"), 0);
      }
    };
  //}}}
  //{{{
  class cOpenGLES3Yuv420Shader : public cTextureShader {
  public:
    cOpenGLES3Yuv420Shader() : cTextureShader() {
      const std::string kFragShader =
        "#version 300 es\n"
        "precision mediump float;"
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

    virtual ~cOpenGLES3Yuv420Shader() { glDeleteProgram (mId); }

    virtual void setModelProjection (const cMat4x4& model, const cMat4x4& projection) final {
      glUniformMatrix4fv (glGetUniformLocation (mId, "uModel"), 1, GL_FALSE, (float*)&model);
      glUniformMatrix4fv (glGetUniformLocation (mId, "uProject"), 1, GL_FALSE, (float*)&projection);
      }

    virtual void use() final {
      glUseProgram (mId);
      glUniform1i (glGetUniformLocation (mId, "ySampler"), 0);
      glUniform1i (glGetUniformLocation (mId, "uSampler"), 1);
      glUniform1i (glGetUniformLocation (mId, "vSampler"), 2);
      }
    };
  //}}}

  //{{{
  static uint32_t compileShader (const std::string& vertShaderString, const std::string& fragShaderString) {

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

  std::string mGlslVersion;
  };

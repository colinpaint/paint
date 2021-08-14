// cGraphics-OpenGL.cpp - singleton - ImGui + openGL >= v2.1 + fbo vao extensions
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include "cGraphics.h"

#include <cstdint>
#include <string>

// glad
#include <glad/glad.h>

// imGui
#include <imgui.h>

#include "../graphics/cPointRect.h"
#include "../graphics/cShader.h"
#include "../log/cLog.h"

// OpenGL >= 3.1 has GL_PRIMITIVE_RESTART state
// OpenGL >= 3.3 has glBindSampler()
#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
  #if defined(GL_VERSION_3_2)
    // OpenGL >= 3.2 has glDrawElementsBaseVertex() which GL ES and WebGL don't have.
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
  #endif
#endif

using namespace std;
using namespace fmt;
//}}}

namespace {
  // versions
  int gGlVersion = 0;    // major.minor * 100
  int gGlslVersion = 0;  // major.minor * 100

  // options
  bool gDrawBase = false;
  bool gClipOriginBottomLeft = true;

  // resources
  GLuint gVboHandle = 0;
  GLuint gElementsHandle = 0;
  GLuint gFontTexture = 0;
  cDrawListShader* gShader;

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
  void setRenderState (ImDrawData* drawData, int width, int height, GLuint vertexArrayObject) {

    // disables
    glDisable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_PRIMITIVE_RESTART);

    // enables
    glEnable (GL_SCISSOR_TEST);
    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);

    // set blend = lerp
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation (GL_FUNC_ADD);
    glEnable (GL_BLEND);

    gShader->use();
    glViewport (0, 0, (GLsizei)width, (GLsizei)height);

    // set orthoProject matrix
    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;;
    float B = drawData->DisplayPos.y;;
    if (gClipOriginBottomLeft) // bottom left origin ortho project
      B += drawData->DisplaySize.y;
    else // top left origin ortho project
      T += drawData->DisplaySize.y;
    float orthoProjectMatrix[4][4] = { { 2.0f/(R-L),  0.0f,        0.0f, 0.0f },
                                       { 0.0f,        2.0f/(T-B),  0.0f, 0.0f },
                                       { 0.0f,        0.0f,       -1.0f, 0.0f },
                                       { (R+L)/(L-R), (T+B)/(B-T), 0.0f, 1.0f },
                                     };
    gShader->setMatrix ((float*)orthoProjectMatrix);

    // bind vertex/index buffers and setup attributes for ImDrawVert
    glBindVertexArray (vertexArrayObject);

    glBindBuffer (GL_ARRAY_BUFFER, gVboHandle);
    glBindBuffer (GL_ELEMENT_ARRAY_BUFFER, gElementsHandle);

    glEnableVertexAttribArray (gShader->getAttribLocationVtxPos());
    glVertexAttribPointer (gShader->getAttribLocationVtxPos(), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                           (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));

    glEnableVertexAttribArray (gShader->getAttribLocationVtxUV());
    glVertexAttribPointer (gShader->getAttribLocationVtxUV(), 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                           (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));

    glEnableVertexAttribArray (gShader->getAttribLocationVtxColor());
    glVertexAttribPointer (gShader->getAttribLocationVtxColor(), 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
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
            if (gDrawBase)
              glDrawElementsBaseVertex (GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                        sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                        (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
                                        (GLint)pcmd->VtxOffset);
            else
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
  }

//{{{
bool cGraphics::init (void* device, void* deviceContext, void* swapChain) {

  // get OpenGL version
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
  cLog::log (LOGINFO, format ("OpenGL {} - {}", glVersionString, gGlVersion));

  // set ImGui backend capabilities flags
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

  // get GLSL version
  string glslVersionString = (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION);
  int glslMajor = 0;
  int glslMinor = 0;
  sscanf (glslVersionString.c_str(), "%d.%d", &glslMajor, &glslMinor);
  gGlslVersion = (glslMajor * 100) + glslMinor;
  cLog::log (LOGINFO, format ("GLSL {} - {}", glslVersionString, gGlslVersion));

  cLog::log (LOGINFO, format ("Renderer {}", glGetString (GL_RENDERER)));
  cLog::log (LOGINFO, format ("- Vendor {}", glGetString (GL_VENDOR)));

  cLog::log (LOGINFO, format ("imGui {} - {}", ImGui::GetVersion(), IMGUI_VERSION_NUM));
  cLog::log (LOGINFO, "- hasViewports");

  // create vertex array buffers
  glGenBuffers (1, &gVboHandle);
  glGenBuffers (1, &gElementsHandle);

  createFontTexture();

  // create shader
  gShader = new cDrawListShader (gGlslVersion);

  return true;
  }
//}}}
//{{{
void cGraphics::shutdown() {

  ImGui::DestroyPlatformWindows();

  // destroy vertetx array
  if (gVboHandle)
    glDeleteBuffers (1, &gVboHandle);
  if (gElementsHandle)
    glDeleteBuffers (1, &gElementsHandle);

  // destroy shader
  delete gShader;

  // destroy font texture
  if (gFontTexture) {
    glDeleteTextures (1, &gFontTexture);
    ImGui::GetIO().Fonts->TexID = 0;
    }
  }
//}}}

//{{{
void cGraphics::draw() {
  renderDrawData (ImGui::GetDrawData());
  }
//}}}
//{{{
void cGraphics::windowResized (int width, int height) {
  cLog::log (LOGINFO, format ("windowResized {} {}", width, height));
  }
//}}}

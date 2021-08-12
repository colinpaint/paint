// cGraphics-OpenGL.cpp - singleton - ImGui, openGL3.3 backend
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include "cGraphics.h"

#include <cstdint>
#include <string>

// glad
#include <glad/glad.h>

// imGui
#include <imgui.h>

#include "../graphics/cShader.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;

#if !defined(IMGUI_IMPL_OPENGL_ES2) && !defined(IMGUI_IMPL_OPENGL_ES3)
  #if defined(GL_VERSION_3_1)
    // OpenGL >= 3.1 has GL_PRIMITIVE_RESTART state
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
  #endif

  #if defined(GL_VERSION_3_2)
    // OpenGL >= 3.2 has glDrawElementsBaseVertex() which GL ES and WebGL don't have.
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
  #endif

  #if defined(GL_VERSION_3_3)
    // OpenGL >= 3.3 has glBindSampler()
    #define IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
  #endif
#endif
//}}}

namespace {
  int gGlVersion = 0;    // major.minor * 100
  int gGlslVersion = 0;  // major.minor * 100

  GLuint gVboHandle = 0;
  GLuint gElementsHandle = 0;

  GLuint gFontTexture = 0;
  cDrawListShader* gShader;

  //{{{
  void createFontAtlasTexture() {
  // Build texture atlas

    // load as RGBA 32-bit (75% of the memory is wasted, but default font is so small)
    // because it is more likely to be compatible with user's existing shaders
    // If your ImTextureId represent a higher-level concept than just a GL texture id,
    // consider calling GetTexDataAsAlpha8() instead to save on GPU memory
    uint8_t* pixels;
    int32_t width;
    int32_t height;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);

    // upload texture to graphics system
    glGenTextures (1, &gFontTexture);
    glBindTexture (GL_TEXTURE_2D, gFontTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    #ifdef GL_UNPACK_ROW_LENGTH
      glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    #endif

    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // store our textureId
    ImGui::GetIO().Fonts->TexID = (ImTextureID)(intptr_t)gFontTexture;
    }
  //}}}

  //{{{
  void setRenderState (ImDrawData* drawData, int width, int height, GLuint vertexArrayObject) {

    // disable cull and depth
    glDisable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);

    // enable scissor
    glEnable (GL_SCISSOR_TEST);

    // lerp blend
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation (GL_FUNC_ADD);
    glEnable (GL_BLEND);

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_PRIMITIVE_RESTART
      if (gGlVersion >= 310)
        glDisable (GL_PRIMITIVE_RESTART);
    #endif

    // enable polygonMode
    #ifdef GL_POLYGON_MODE
      glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    #endif

    // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
    bool clip_origin_lower_left = true;
    #if defined (GL_CLIP_ORIGIN)
      GLenum current_clip_origin = 0;
      glGetIntegerv (GL_CLIP_ORIGIN, (GLint*)&current_clip_origin);
      if (current_clip_origin == GL_UPPER_LEFT)
        clip_origin_lower_left = false;
    #endif

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from
    // - drawData->DisplayPos (top left) to drawData->DisplayPos+data_data->DisplaySize (bottom right)
    // - DisplayPos is (0,0) for single viewport apps
    glViewport (0, 0, (GLsizei)width, (GLsizei)height);
    float L = drawData->DisplayPos.x;
    float R = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float T = drawData->DisplayPos.y;
    float B = drawData->DisplayPos.y + drawData->DisplaySize.y;
    if (!clip_origin_lower_left) {
      //{{{  Swap top and bottom if origin is upper left
      float tmp = T;
      T = B;
      B = tmp;
      }
      //}}}

    float ortho_projection[4][4] = { { 2.0f/(R-L),  0.0f,        0.0f, 0.0f },
                                     { 0.0f,        2.0f/(T-B),  0.0f, 0.0f },
                                     { 0.0f,        0.0f,       -1.0f, 0.0f },
                                     { (R+L)/(L-R), (T+B)/(B-T), 0.0f, 1.0f },
                                   };
    gShader->use();
    //glUniform1i (gAttribLocationTex, 0); // what did thus do ?
    gShader->setMatrix ((float*)ortho_projection);

    #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_BIND_SAMPLER
      // test use combined texture/sampler state. Applications using GL 3.3 may set that otherwise.
      if (gGlVersion >= 330)
        glBindSampler (0, 0);
    #endif

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
    // - scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (width <= 0 || height <= 0)
      return;

    // temp VAO
    GLuint vertexArrayObject;
    glGenVertexArrays (1, &vertexArrayObject);

    // setup desired GL state
    setRenderState (drawData, width, height, vertexArrayObject);

    // project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // render command lists
    for (int n = 0; n < drawData->CmdListsCount; n++) {
      const ImDrawList* cmdList = drawData->CmdLists[n];

      // upload vertex/index buffers
      glBufferData (GL_ARRAY_BUFFER, (GLsizeiptr)cmdList->VtxBuffer.Size * (int)sizeof(ImDrawVert),
                    (const GLvoid*)cmdList->VtxBuffer.Data, GL_STREAM_DRAW);
      glBufferData (GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmdList->IdxBuffer.Size * (int)sizeof(ImDrawIdx),
                    (const GLvoid*)cmdList->IdxBuffer.Data, GL_STREAM_DRAW);

      for (int cmd_i = 0; cmd_i < cmdList->CmdBuffer.Size; cmd_i++) {
        const ImDrawCmd* pcmd = &cmdList->CmdBuffer[cmd_i];
        if (pcmd->UserCallback != NULL) {
          // user callback, registered via ImDrawList::AddCallback()
          // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
          if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            setRenderState (drawData, width, height, vertexArrayObject);
          else
            pcmd->UserCallback (cmdList, pcmd);
          }
        else {
          // project scissor/clipping rectangles into framebuffer space
          ImVec4 clip_rect;
          clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
          clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
          clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
          clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

          if (clip_rect.x < width && clip_rect.y < height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
            // apply scissor/clipping rectangle
            glScissor ((int)clip_rect.x, (int)(height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

            // bind texture, draw
            glBindTexture (GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->TextureId);

            #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
              if (gGlVersion >= 320)
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
      }

    glDeleteVertexArrays (1, &vertexArrayObject);
    }
  //}}}
  //{{{
  void renderWindow (ImGuiViewport* viewport, void*) {

    if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear)) {
      ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
      glClearColor (clear_color.x, clear_color.y, clear_color.z, clear_color.w);
      glClear (GL_COLOR_BUFFER_BIT);
      }

    renderDrawData (viewport->DrawData);
    }
  //}}}
  }

//{{{
bool cGraphics::init() {

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
  #ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
    if (gGlVersion >= 320) {
      ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
      cLog::log (LOGINFO, "- hasVtxOffset");
      }
  #endif

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

  createFontAtlasTexture();

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

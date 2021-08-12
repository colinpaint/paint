// cGraphics-OpenGL2.cpp - singleton - ImGui, openGL 2.1 backend
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#include "cGraphics.h"

#include <cstdint>
#include <string>

// glad
#include <glad/glad.h>

// imGui
#include <imgui.h>

#include "../log/cLog.h"

// Include OpenGL header (without an OpenGL loader) requires a bit of fiddling
#if defined(_WIN32) && !defined(APIENTRY)
  #define APIENTRY __stdcall                  // It is customary to use APIENTRY for OpenGL function pointer declarations on all platforms.  Additionally, the Windows OpenGL header needs APIENTRY.
#endif

#if defined(_WIN32) && !defined(WINGDIAPI)
  #define WINGDIAPI __declspec(dllimport)     // Some Windows OpenGL headers need this
#endif

//#include <GL/gl.h>

using namespace std;
using namespace fmt;
//}}}

namespace {
  int gGlVersion = 0;    // major.minor * 100
  int gGlslVersion = 0;  // major.minor * 100

  //{{{
  struct sBackendData {
    GLuint FontTexture;
    sBackendData() { memset(this, 0, sizeof(*this)); }
    };
  //}}}
  //{{{
  // Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
  // It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
  sBackendData* getBackendData() {

    return ImGui::GetCurrentContext() ? (sBackendData*)ImGui::GetIO().BackendRendererUserData : NULL;
    }
  //}}}

  //{{{
  bool createFontsTexture() {

    // build texture atlas
    ImGuiIO& io = ImGui::GetIO();
    sBackendData* backendData = getBackendData();
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32 (&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    // upload texture to graphics system
    GLint last_texture;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &last_texture);
    glGenTextures (1, &backendData->FontTexture);
    glBindTexture (GL_TEXTURE_2D, backendData->FontTexture);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // store our identifier
    io.Fonts->SetTexID ((ImTextureID)(intptr_t)backendData->FontTexture);

    // restore state
    glBindTexture (GL_TEXTURE_2D, last_texture);

    return true;
    }
  //}}}
  //{{{
  void destroyFontsTexture() {

    ImGuiIO& io = ImGui::GetIO();
    sBackendData* backendData = getBackendData();
    if (backendData->FontTexture) {
      glDeleteTextures (1, &backendData->FontTexture);
      io.Fonts->SetTexID(0);
      backendData->FontTexture = 0;
      }
    }
  //}}}

  //{{{
  void setupRenderState (ImDrawData* draw_data, int fb_width, int fb_height) {

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers, polygon fill.
    glEnable (GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // In order to composite our output buffer we need to preserve alpha

    glDisable (GL_CULL_FACE);
    glDisable (GL_DEPTH_TEST);
    glDisable (GL_STENCIL_TEST);
    glDisable (GL_LIGHTING);
    glDisable (GL_COLOR_MATERIAL);

    glEnable (GL_SCISSOR_TEST);
    glEnableClientState (GL_VERTEX_ARRAY);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glEnableClientState (GL_COLOR_ARRAY);
    glDisableClientState (GL_NORMAL_ARRAY);
    glEnable (GL_TEXTURE_2D);

    glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
    glShadeModel (GL_SMOOTH);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // If you are using this code with non-legacy OpenGL header/contexts (which you should not, prefer using imgui_impl_opengl3.cpp!!),
    // you may need to backup/reset/restore other state, e.g. for current shader using the commented lines below.
    // (DO NOT MODIFY THIS FILE! Add the code in your calling function)
    //   GLint last_program;
    //   glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
    //   glUseProgram(0);
    //   RenderDrawData(...);
    //   glUseProgram(last_program)
    // There are potentially many more states you could need to clear/setup that we can't access from default headers.
    // e.g. glBindBuffer(GL_ARRAY_BUFFER, 0), glDisable(GL_TEXTURE_CUBE_MAP).

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    glViewport (0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
    glMatrixMode (GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho (draw_data->DisplayPos.x, draw_data->DisplayPos.x + draw_data->DisplaySize.x, draw_data->DisplayPos.y + draw_data->DisplaySize.y, draw_data->DisplayPos.y, -1.0f, +1.0f);
    glMatrixMode (GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    }
  //}}}
  //{{{
  void renderDrawData (ImDrawData* draw_data) {
  // OpenGL2 Render function.
  // Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly.
  // This is in order to be able to run within an OpenGL engine that doesn't do so.

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    // Backup GL state
    GLint last_texture;
    glGetIntegerv (GL_TEXTURE_BINDING_2D, &last_texture);
    GLint last_polygon_mode[2];
    glGetIntegerv (GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4];
    glGetIntegerv (GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4];
    glGetIntegerv (GL_SCISSOR_BOX, last_scissor_box);
    GLint last_shade_model;
    glGetIntegerv (GL_SHADE_MODEL, &last_shade_model);
    GLint last_tex_env_mode;
    glGetTexEnviv (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &last_tex_env_mode);
    glPushAttrib (GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);

    // Setup desired GL state
    setupRenderState (draw_data, fb_width, fb_height);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
      const ImDrawList* cmd_list = draw_data->CmdLists[n];
      const ImDrawVert* vtx_buffer = cmd_list->VtxBuffer.Data;
      const ImDrawIdx* idx_buffer = cmd_list->IdxBuffer.Data;
      glVertexPointer (2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, pos)));
      glTexCoordPointer (2, GL_FLOAT, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, uv)));
      glColorPointer (4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (const GLvoid*)((const char*)vtx_buffer + IM_OFFSETOF(ImDrawVert, col)));

      for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
        if (pcmd->UserCallback) {
          // User callback, registered via ImDrawList::AddCallback()
          // (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
          if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
            setupRenderState (draw_data, fb_width, fb_height);
          else
            pcmd->UserCallback (cmd_list, pcmd);
          }
        else {
          // Project scissor/clipping rectangles into framebuffer space
          ImVec4 clip_rect;
          clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
          clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
          clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
          clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

          if (clip_rect.x < fb_width && clip_rect.y < fb_height &&
              clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
            // Apply scissor/clipping rectangle
            glScissor((int)clip_rect.x, (int)(fb_height - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));

            // Bind texture, Draw
            glBindTexture (GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
            glDrawElements (GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer);
            }
          }
        idx_buffer += pcmd->ElemCount;
        }
      }

    // Restore modified GL state
    glDisableClientState (GL_COLOR_ARRAY);
    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glDisableClientState (GL_VERTEX_ARRAY);
    glBindTexture (GL_TEXTURE_2D, (GLuint)last_texture);
    glMatrixMode (GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode (GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glPolygonMode (GL_FRONT, (GLenum)last_polygon_mode[0]); glPolygonMode(GL_BACK, (GLenum)last_polygon_mode[1]);
    glViewport (last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
    glScissor (last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    glShadeModel (last_shade_model);
    glTexEnvi (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, last_tex_env_mode);
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
  GLint glMajor = 0;
  GLint glMinor = 0;
  sscanf (glVersionString.c_str(), "%d.%d", &glMajor, &glMinor);
  gGlVersion = ((glMajor * 10) + glMinor) * 10;
  cLog::log (LOGINFO, format ("OpenGL {} - {}", glVersionString, gGlVersion));

  string glslVersionString = (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION);
  int glslMajor = 0;
  int glslMinor = 0;
  sscanf (glslVersionString.c_str(), "%d.%d", &glslMajor, &glslMinor);
  gGlslVersion = (glslMajor * 100) + glslMinor;
  cLog::log (LOGINFO, format ("GLSL {} - {}", glslVersionString, gGlslVersion));
  cLog::log (LOGINFO, format ("Renderer {}", glGetString (GL_RENDERER)));
  cLog::log (LOGINFO, format ("- Vendor {}", glGetString (GL_VENDOR)));

  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT (io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

  // Setup backend capabilities flags
  sBackendData* backendData = IM_NEW(sBackendData)();
  io.BackendRendererUserData = (void*)backendData;
  io.BackendRendererName = "imgui_impl_opengl2";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;    // We can create multi-viewports on the Renderer side (optional)

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Renderer_RenderWindow = renderWindow;
    }

  createFontsTexture();

  return true;
  }
//}}}
//{{{
void cGraphics::shutdown() {

  ImGuiIO& io = ImGui::GetIO();
  sBackendData* backendData = getBackendData();

  ImGui::DestroyPlatformWindows();
  destroyFontsTexture();

  io.BackendRendererName = NULL;
  io.BackendRendererUserData = NULL;
  IM_DELETE (backendData);
  }
//}}}

//{{{
void cGraphics::draw() {
  renderDrawData (ImGui::GetDrawData());
  }
//}}}

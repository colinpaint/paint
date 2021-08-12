//{{{  includes
#include "imgui.h"
#include "imgui_impl_opengl2.h"

#include <stdint.h>     // intptr_t

// Include OpenGL header (without an OpenGL loader) requires a bit of fiddling
#if defined(_WIN32) && !defined(APIENTRY)
  #define APIENTRY __stdcall                  // It is customary to use APIENTRY for OpenGL function pointer declarations on all platforms.  Additionally, the Windows OpenGL header needs APIENTRY.
#endif

#if defined(_WIN32) && !defined(WINGDIAPI)
  #define WINGDIAPI __declspec(dllimport)     // Some Windows OpenGL headers need this
#endif

#include <GL/gl.h>
//}}}

//{{{
struct ImGui_ImplOpenGL2_Data {
  GLuint FontTexture;
  ImGui_ImplOpenGL2_Data() { memset(this, 0, sizeof(*this)); }
  };
//}}}
//{{{
// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplOpenGL2_Data* ImGui_ImplOpenGL2_GetBackendData() {

  return ImGui::GetCurrentContext() ? (ImGui_ImplOpenGL2_Data*)ImGui::GetIO().BackendRendererUserData : NULL;
  }
//}}}

//{{{
bool ImGui_ImplOpenGL2_CreateFontsTexture() {

  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();
  ImGui_ImplOpenGL2_Data* bd = ImGui_ImplOpenGL2_GetBackendData();
  unsigned char* pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

  // Upload texture to graphics system
  GLint last_texture;
  glGetIntegerv (GL_TEXTURE_BINDING_2D, &last_texture);
  glGenTextures (1, &bd->FontTexture);
  glBindTexture (GL_TEXTURE_2D, bd->FontTexture);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

  // Store our identifier
  io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

  // Restore state
  glBindTexture (GL_TEXTURE_2D, last_texture);

  return true;
  }
//}}}
//{{{
void ImGui_ImplOpenGL2_DestroyFontsTexture() {

  ImGuiIO& io = ImGui::GetIO();
  ImGui_ImplOpenGL2_Data* bd = ImGui_ImplOpenGL2_GetBackendData();
  if (bd->FontTexture) {
    glDeleteTextures (1, &bd->FontTexture);
    io.Fonts->SetTexID(0);
    bd->FontTexture = 0;
    }
  }
//}}}

//{{{
bool ImGui_ImplOpenGL2_CreateDeviceObjects() {
  return ImGui_ImplOpenGL2_CreateFontsTexture();
  }
//}}}
//{{{
void ImGui_ImplOpenGL2_DestroyDeviceObjects() {
  ImGui_ImplOpenGL2_DestroyFontsTexture();
  }
//}}}

//{{{
static void ImGui_ImplOpenGL2_SetupRenderState (ImDrawData* draw_data, int fb_width, int fb_height) {

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
  //   ImGui_ImplOpenGL2_RenderDrawData(...);
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
void ImGui_ImplOpenGL2_RenderDrawData (ImDrawData* draw_data) {
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
  ImGui_ImplOpenGL2_SetupRenderState (draw_data, fb_width, fb_height);

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
          ImGui_ImplOpenGL2_SetupRenderState (draw_data, fb_width, fb_height);
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
static void ImGui_ImplOpenGL2_RenderWindow (ImGuiViewport* viewport, void*) {

  if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear)) {
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    glClearColor (clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear (GL_COLOR_BUFFER_BIT);
    }

  ImGui_ImplOpenGL2_RenderDrawData(viewport->DrawData);
  }
//}}}
//{{{
static void ImGui_ImplOpenGL2_InitPlatformInterface() {

  ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
  platform_io.Renderer_RenderWindow = ImGui_ImplOpenGL2_RenderWindow;
  }
//}}}
//{{{
static void ImGui_ImplOpenGL2_ShutdownPlatformInterface() {
  ImGui::DestroyPlatformWindows();
  }
//}}}

//{{{
bool ImGui_ImplOpenGL2_Init() {

  ImGuiIO& io = ImGui::GetIO();
  IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");

  // Setup backend capabilities flags
  ImGui_ImplOpenGL2_Data* bd = IM_NEW(ImGui_ImplOpenGL2_Data)();
  io.BackendRendererUserData = (void*)bd;
  io.BackendRendererName = "imgui_impl_opengl2";
  io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;    // We can create multi-viewports on the Renderer side (optional)

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
      ImGui_ImplOpenGL2_InitPlatformInterface();

  return true;
  }
//}}}
//{{{
void ImGui_ImplOpenGL2_Shutdown() {

  ImGuiIO& io = ImGui::GetIO();
  ImGui_ImplOpenGL2_Data* bd = ImGui_ImplOpenGL2_GetBackendData();

  ImGui_ImplOpenGL2_ShutdownPlatformInterface();
  ImGui_ImplOpenGL2_DestroyDeviceObjects();
  io.BackendRendererName = NULL;
  io.BackendRendererUserData = NULL;
  IM_DELETE(bd);
  }
//}}}

//{{{
void ImGui_ImplOpenGL2_NewFrame() {

  ImGui_ImplOpenGL2_Data* bd = ImGui_ImplOpenGL2_GetBackendData();

  IM_ASSERT(bd != NULL && "Did you call ImGui_ImplOpenGL2_Init()?");

  if (!bd->FontTexture)
    ImGui_ImplOpenGL2_CreateDeviceObjects();
  }
//}}}

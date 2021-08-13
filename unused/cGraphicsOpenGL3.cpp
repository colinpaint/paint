// cGraphics.cpp - ImGui, openGL3.3
//{{{  includes
#include "../graphics/cGraphics.h"

#include <cstdint>
#include <string>

// glad
#include <glad/glad.h>

// imGui
#include <imgui.h>

//  - imGui openGL3 backend
#include <imgui_impl_opengl3.h>

#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

bool cGraphics::init() {
  if (ImGui_ImplOpenGL3_Init ("#version 330 core")) {
    const char* glVersion = (const char*)glGetString (GL_VERSION);
    cLog::log (LOGINFO, format ("OpenGL - version {}", glVersion));

    string glslVersion = (const char*)glGetString (GL_SHADING_LANGUAGE_VERSION);
    cLog::log (LOGINFO, format ("GLSL - version {}", glslVersion));

    string glRenderer = (const char*)glGetString (GL_RENDERER);
    cLog::log (LOGINFO, format ("Renderer - {}", glRenderer));

    string glVendor = (const char*)glGetString (GL_VENDOR);
    cLog::log (LOGINFO, format ("Vendor - {}", glVendor));
    return true;
    }

  cLog::log (LOGERROR, "ImGui_ImplOpenGL3_Init failed");
  return false;
  }

void cGraphics::shutdown() {
  ImGui_ImplOpenGL3_Shutdown();
  }

void cGraphics::newFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  }

void cGraphics::draw() {
  ImGui_ImplOpenGL3_RenderDrawData (ImGui::GetDrawData());
  }

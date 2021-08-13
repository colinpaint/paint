// cPlatform-win32-wrapper.cpp - imGui backend win32Impl wrapper - abstracted from imGui win32DirectX11 example main.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cPlatform.h"

#include <cstdint>
#include <string>

// glm
#include <vec2.hpp>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_win32.h>

#include "../graphics/cPointRect.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  // vars
  GLFWwindow* gWindow = nullptr;
  cPlatform::sizeCallbackFunc gSizeCallback;

  // glfw callbacks
  //{{{
  void keyCallback (GLFWwindow* window, int key, int scancode, int action, int mode) {

    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) // exit
      glfwSetWindowShouldClose (window, true);
    else if (key >= 0 && key < 1024)
      if (action == GLFW_RELEASE) {
        // other user key actions
        }
    }
  //}}}
  //{{{
  void framebufferSizeCallback (GLFWwindow* window, int width, int height) {
    gSizeCallback();
    }
  //}}}
  }

//{{{
bool cPlatform::init (const cPoint& windowSize, bool showViewports, const sizeCallbackFunc sizeCallback) {

  gSizeCallback = sizeCallback;

  return true;
  }
//}}}
//{{{
void cPlatform::shutdown() {
  }
//}}}

// gets
//{{{
cPoint cPlatform::getWindowSize() {

  return cPoint();
  }
//}}}

// actions
//{{{
bool cPlatform::pollEvents() {

  return true;
  }
//}}}
//{{{
void cPlatform::newFrame() {
  }
//}}}
//{{{
void cPlatform::present() {
  }
//}}}

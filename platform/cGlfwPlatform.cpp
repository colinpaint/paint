// cGlfwPlatform.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>

// glad
#include <glad/glad.h>

// glfw
#include <GLFW/glfw3.h>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#ifdef USE_IMPLOT
  #include <implot.h>
#endif

#include "cPlatform.h"
#include "../utils/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  cPlatform* gPlatform = nullptr;
  GLFWwindow* gWindow = nullptr;

  GLFWmonitor* gMonitor = nullptr;
  cPoint gWindowPos = { 0,0 };
  cPoint gWindowSize = { 0,0 };

  //{{{
  void keyCallback (GLFWwindow* window, int key, int scancode, int action, int mode) {

    (void)scancode;
    (void)mode;
    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS))
      // exit
      glfwSetWindowShouldClose (window, true);

    else if ((key == GLFW_KEY_F) && (action == GLFW_PRESS)) {
      // toggle fullscreen
      if (glfwGetWindowMonitor (window) == nullptr) {
        // save windowPos and windowSize
        glfwGetWindowPos (window, &gWindowPos.x, &gWindowPos.y);
        glfwGetWindowSize (window, &gWindowSize.x, &gWindowSize.y);

        // get resolution of monitor
        const GLFWvidmode* vidMode = glfwGetVideoMode (glfwGetPrimaryMonitor());

        // switch to full screen
        glfwSetWindowMonitor  (window, gMonitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
        }
      else
        // restore last window size and position
        glfwSetWindowMonitor (window, nullptr, gWindowPos.x, gWindowPos.y, gWindowSize.x, gWindowSize.y, 0);
      }

    else if (key >= 0 && key < 1024)
      if (action == GLFW_RELEASE) {
        // other user key actions
        }
    }
  //}}}
  //{{{
  void framebufferSizeCallback (GLFWwindow* window, int width, int height) {

    (void)window;
    if (gPlatform)
      gPlatform->mResizeCallback (width, height);
    }
  //}}}
  }

// cGlfwPlatform class header, easy to extract header if static register not ok
//{{{
class cGlfwPlatform : public cPlatform {
public:
  void shutdown() final;

  // gets
  void* getDevice() final;
  void* getDeviceContext() final;
  void* getSwapChain() final;
  cPoint getWindowSize() final;

  // actions
  bool pollEvents() final;
  void newFrame() final;
  void present() final;

protected:
  bool init (const cPoint& windowSize, bool showViewports, bool vsync) final;

private:
  // static register
  static cPlatform* create (const std::string& className);
  inline static const bool mRegistered = registerClass ("glfw", &create);
  };
//}}}

// public:
//{{{
void cGlfwPlatform::shutdown() {

  ImGui_ImplGlfw_Shutdown();

  glfwDestroyWindow (gWindow);
  glfwTerminate();

  #ifdef USE_IMPLOT
    ImPlot::DestroyContext();
  #endif
  ImGui::DestroyContext();
  }
//}}}

// gets
void* cGlfwPlatform::getDevice() { return nullptr; }
void* cGlfwPlatform::getDeviceContext() { return nullptr; }
void* cGlfwPlatform::getSwapChain() { return nullptr; }
//{{{
cPoint cGlfwPlatform::getWindowSize() {

  int width;
  int height;
  glfwGetWindowSize (gWindow, &width, &height);
  return cPoint (width, height);
  }
//}}}

// actions
//{{{
bool cGlfwPlatform::pollEvents() {

  if (glfwWindowShouldClose (gWindow))
    return false;
  else {
    glfwPollEvents();
    return true;
    }
  }
//}}}
//{{{
void cGlfwPlatform::newFrame() {
  ImGui_ImplGlfw_NewFrame();
  }
//}}}
//{{{
void cGlfwPlatform::present() {

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent (backupCurrentContext);
    }

  glfwSwapBuffers (gWindow);
  }
//}}}

// protected:
//{{{
bool cGlfwPlatform::init (const cPoint& windowSize, bool showViewports, bool vsync) {

  cLog::log (LOGINFO, format ("GLFW {}.{}", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR));

  // GLFW init
  if (!glfwInit()) {
    cLog::log (LOGERROR, "cPlatform - glfw init failed");
    return false;
    }

  // openGL version hints
  #if defined(OPENGL_21)
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);
    cLog::log (LOGINFO, "- version hint 2.1");
  #elif defined(OPENGL_40)
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
    cLog::log (LOGINFO, "- version hint 4.0");
  #elif defined(OPENGL_45)
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 5);
    cLog::log (LOGINFO, "- version hint 4.5");
  #else
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
    cLog::log (LOGINFO, "- version hint 3.3");
  #endif
  //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // GLFW create window
  gWindow = glfwCreateWindow (windowSize.x, windowSize.y, "Paintbox - openGL", NULL, NULL);
  if (!gWindow) {
    cLog::log (LOGERROR, "cPlatform - glfwCreateWindow failed");
    return false;
    }

  gMonitor = glfwGetPrimaryMonitor();
  glfwGetWindowSize (gWindow, &gWindowSize.x, &gWindowSize.y);
  glfwGetWindowPos (gWindow, &gWindowPos.x, &gWindowPos.y);

  glfwMakeContextCurrent (gWindow);

  // set callbacks
  glfwSetKeyCallback (gWindow, keyCallback);
  glfwSetFramebufferSizeCallback (gWindow, framebufferSizeCallback);

  // GLAD init before any openGL function
  if (!gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress)) {
    cLog::log (LOGERROR, "cPlatform - glad init failed");
    return false;
    }

  // init imGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsClassic();
  #ifdef USE_IMPLOT
    ImPlot::CreateContext();
  #endif

  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  if (showViewports)
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui_ImplGlfw_InitForOpenGL (gWindow, true);

  glfwSwapInterval (vsync ? 1 : 0);

  gPlatform = this;

  return true;
  }
//}}}

// private:
//{{{
cPlatform* cGlfwPlatform::create (const string& className) {

  (void)className;
  return new cGlfwPlatform();
  }
//}}}

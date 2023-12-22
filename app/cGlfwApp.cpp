// cGlfwApp.cpp - glfw + openGL3,openGLES3_x
//{{{  includes
#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
  #include <windows.h>
#endif

#include "cApp.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <array>
#include <algorithm>
#include <functional>
#include <thread>

// glad
#if defined(GL3)
  #include <glad/glad.h>
#endif

// glfw
#include <GLFW/glfw3.h>

// imGui
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"

#if defined(GL3)
  #include "cGL3Graphics.h"
#else
  #include "cGLES3Graphics.h"
#endif

// app
#include "cPlatform.h"
#include "cGraphics.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// utils
#include "../common/cLog.h"

using namespace std;
//}}}

// cPlatform interface
//{{{
class cGlfwPlatform : public cPlatform {
public:
  cGlfwPlatform (const string& name) : cPlatform (name, true, true) {}
  //{{{
  virtual ~cGlfwPlatform() {

    ImGui_ImplGlfw_Shutdown();

    glfwDestroyWindow (mWindow);
    glfwTerminate();

    ImGui::DestroyContext();
    }
  //}}}

  //{{{
  virtual bool init (const cPoint& windowSize) final {

    //  select openGL, openGLES version
    #if defined(GL3)
      //{{{  GL3.3, GLSL130
      string title = "GL3";
      const char* glslVersion = "#version 130";

      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 3);
      //glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
      //glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
      //}}}
    #elif defined(GLES30)
      //{{{  GLES3.0, GLSL300
      string title = "GLES3.0";
      const char* glslVersion = "#version 300 es";

      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 0);
      glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      //}}}
    #elif defined(GLES31)
      //{{{  GLES3.1, GLSL300
      string title = "GLES3.1";
      const char* glslVersion = "#version 300 es";

      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 1);
      glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      //}}}
    #elif defined(GLES32)
      //{{{  GLES3.2, GLSL300
      string title = "GLES3.2";
      const char* glslVersion = "#version 300 es";

      glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
      glfwWindowHint (GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
      //}}}
    #endif

    cLog::log (LOGINFO, fmt::format ("glfw {}.{} using {} glsl {}",
                                     GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, title, glslVersion));

    glfwSetErrorCallback (glfwErrorCallback);
    if (!glfwInit())
      return false;

    setShaderVersion (glslVersion);

    mWindow = glfwCreateWindow (windowSize.x, windowSize.y, (title + " " + getName()).c_str(), NULL, NULL);
    if (!mWindow) {
      //{{{  error, log, return
      cLog::log (LOGERROR, "cGlfwPlatform - glfwCreateWindow failed");
      return false;
      }
      //}}}

    mMonitor = glfwGetPrimaryMonitor();
    glfwGetWindowSize (mWindow, &mWindowSize.x, &mWindowSize.y);
    glfwGetWindowPos (mWindow, &mWindowPos.x, &mWindowPos.y);
    glfwMakeContextCurrent (mWindow);

    glfwSwapInterval (1);

    // set imGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsClassic();
    cLog::log (LOGINFO, fmt::format ("imGui {} - {}", ImGui::GetVersion(), IMGUI_VERSION_NUM));

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // set our callbacks
    glfwSetKeyCallback (mWindow, keyCallback);
    glfwSetFramebufferSizeCallback (mWindow, framebufferSizeCallback);
    glfwSetDropCallback (mWindow, dropCallback);

    //
    ImGui_ImplGlfw_InitForOpenGL (mWindow, true);

    #if defined(GL3)
      // openGL - GLAD init before any openGL function
      if (!gladLoadGLLoader ((GLADloadproc)glfwGetProcAddress)) {
        cLog::log (LOGERROR, "cOpenGL3Platform - gladLoadGLLoader failed");
        return false;
        }
    #endif

    return true;
    }
  //}}}

  // gets
  //{{{
  virtual cPoint getWindowSize() final {

    int width;
    int height;
    glfwGetWindowSize (mWindow, &width, &height);

    return cPoint (width, height);
    }
  //}}}

  // sets
  //{{{
  virtual void setVsync (bool vsync) final {

    if (mVsync != vsync) {
      mVsync = vsync;
      setSwapInterval (mVsync);
      }
    }
  //}}}
  //{{{
  virtual void toggleVsync() final {

    mVsync = !mVsync;
    setSwapInterval (mVsync);
    }
  //}}}

  //{{{
  virtual void setFullScreen (bool fullScreen) final {

    if (mFullScreen != fullScreen) {
      mFullScreen = fullScreen;
      mActionFullScreen = true;
      }
    }
  //}}}
  //{{{
  virtual void toggleFullScreen() final {

    mFullScreen = !mFullScreen;
    mActionFullScreen = true;
    }
  //}}}

  // actions
  //{{{
  virtual bool pollEvents() final {

    if (mActionFullScreen) {
      // fullScreen
      if (mFullScreen) {
        // save windowPos and windowSize
        glfwGetWindowPos (mWindow, &mWindowPos.x, &mWindowPos.y);
        glfwGetWindowSize (mWindow, &mWindowSize.x, &mWindowSize.y);

        // get resolution of monitor
        const GLFWvidmode* vidMode = glfwGetVideoMode (glfwGetPrimaryMonitor());

        // switch to full screen
        glfwSetWindowMonitor  (mWindow, mMonitor, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
        }
      else
        // restore last window size and position
        glfwSetWindowMonitor (mWindow, nullptr, mWindowPos.x, mWindowPos.y, mWindowSize.x, mWindowSize.y, 0);

      mActionFullScreen = false;
      }

    if (glfwWindowShouldClose (mWindow))
      return false;

    glfwPollEvents();
    return true;
    }
  //}}}
  virtual void newFrame() final { ImGui_ImplGlfw_NewFrame(); }
  virtual void present() final { glfwSwapBuffers (mWindow); }

  // static for glfw callback
  inline static function <void (int width, int height)> mResizeCallback ;
  inline static function <void (vector<string> dropItems)> mDropCallback;

private:
  void setSwapInterval (bool vsync) { glfwSwapInterval (vsync ? 1 : 0); }

  // static for glfw callback
  //{{{
  static void glfwErrorCallback (int error, const char* description) {
    cLog::log (LOGERROR, fmt::format ("glfw failed {} {}", error, description));
    }
  //}}}
  //{{{
  static void keyCallback (GLFWwindow* window, int key, int scancode, int action, int mode) {
  // glfw key callback exit
    (void)scancode;
    (void)mode;

    if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) // exit
      glfwSetWindowShouldClose (window, true);
    }
  //}}}
  //{{{
  static void framebufferSizeCallback (GLFWwindow* window, int width, int height) {

    (void)window;
    mResizeCallback (width, height);
    }
  //}}}
  //{{{
  static void dropCallback (GLFWwindow* window, int count, const char** paths) {

    (void)window;
    vector<string> dropItems;
    for (int i = 0;  i < count;  i++)
      dropItems.push_back (paths[i]);

    mDropCallback (dropItems);
    }
  //}}}

  // window vars
  GLFWmonitor* mMonitor = nullptr;
  GLFWwindow* mWindow = nullptr;
  cPoint mWindowPos = { 0,0 };
  cPoint mWindowSize = { 0,0 };
  bool mActionFullScreen = false;
  };
//}}}

// cApp
//{{{
cApp::cApp (const string& name, cApp::cOptions* options, iUI* ui) :
    mName(name), mOptions(options), mUI(ui) {

  if (options->mHasGui) {
    // create GLFW platform
    cGlfwPlatform* glfwPlatform = new cGlfwPlatform (name);
    if (!glfwPlatform || !glfwPlatform->init (options->mWindowSize)) {
      cLog::log (LOGERROR, "cApp - glfwPlatform init failed");
      return;
      }

    // create OpenGL graphics
    #if defined(GL3)
      mGraphics = new cGL3Graphics (glfwPlatform->getShaderVersion());
    #else
      mGraphics = new cGLES3Graphics (glfwPlatform->getShaderVersion());
    #endif

    if (mGraphics && mGraphics->init()) {
      // set callbacks
      glfwPlatform->mResizeCallback = [&](int width, int height) noexcept { windowResize (width, height); };
      glfwPlatform->mDropCallback = [&](vector<string> dropItems) noexcept { drop (dropItems); };

      // fullScreen, vsync
      mPlatform = glfwPlatform;
      mPlatform->setFullScreen (options->mFullScreen);
      mPlatform->setVsync (options->mVsync);
      }
    else
      cLog::log (LOGERROR, "cApp - graphics init failed");

    setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 16.f));
    setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 16.f));
    }
  }
//}}}
//{{{
cApp::~cApp() {
  delete mGraphics;
  delete mPlatform;
  }
//}}}

// get
//{{{
chrono::system_clock::time_point cApp::getNow() {
// get time_point with daylight saving correction
// - should be a C++20 timezone thing, but not yet

  // get daylight saving flag
  time_t current_time;
  time (&current_time);
  struct tm* timeinfo = localtime (&current_time);
  //cLog::log (LOGINFO, fmt::format ("dst {}", timeinfo->tm_isdst));

  // UTC->BST only
  return std::chrono::system_clock::now() + std::chrono::hours ((timeinfo->tm_isdst == 1) ? 1 : 0);
  }
//}}}

// callback
//{{{
void cApp::windowResize (int width, int height) {
  (void)width;
  (void)height;

  mGraphics->newFrame();
  mPlatform->newFrame();
  ImGui::NewFrame();

  mUI->draw (*this, *mGraphics);
  ImGui::Render();
  mGraphics->renderDrawData();

  mPlatform->present();
  }
//}}}

// main loop
//{{{
void cApp::mainUILoop() {

  if (mPlatform && mGraphics && mUI) {
    while (mPlatform->pollEvents()) {
      mGraphics->newFrame();
      mPlatform->newFrame();
      ImGui::NewFrame();

      mUI->draw (*this, *mGraphics);
      ImGui::Render();
      mGraphics->renderDrawData();

      mPlatform->present();
      }
    }

  else // headless
    while (true)
      this_thread::sleep_for (100ms);
  }
//}}}

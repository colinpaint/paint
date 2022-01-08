// cSdlApp.cpp - glfw + openGL2,openGL3,openGLES3_x,vulkan app framework
//{{{  includes
#if defined(_WIN32)
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
  #include <windows.h>
#endif

#include <cstdint>
#include <cmath>
#include <string>
#include <array>
#include <algorithm>
#include <functional>

// app
#include "cApp.h"
#include "cPlatform.h"
#include "cGraphics.h"

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_opengl.h>

// imGui
#include <imgui.h>
#include <backends/imgui_impl_sdl.h>
#include <backends/imgui_impl_opengl3.h>

//#include "cGL3Graphics.h"

#include "../ui/cUI.h"

#include "../utils/cLog.h"

using namespace std;
//}}}

// cPlatform interface
//{{{
class cSdlPlatform : public cPlatform {
public:
  cSdlPlatform (const string& name) : cPlatform (name, true, true) {}
  //{{{
  virtual ~cSdlPlatform() {

    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext (mGlContext);
    SDL_DestroyWindow (mWindow);
    SDL_Quit();
    }
  //}}}

  //{{{
  virtual bool init (const cPoint& windowSize) final {

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
      return false;

    //const char* glsl_version = "#version 130";

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    mWindow = SDL_CreateWindow ("SDL2 OpenGL3",
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, windowSize.x, windowSize.y, window_flags);

    mGlContext = SDL_GL_CreateContext (mWindow);
    SDL_GL_MakeCurrent (mWindow, mGlContext);

    SDL_GL_SetSwapInterval (1); // Enable vsync
    //  SDL_GL_SetSwapInterval (0);

    // set imGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsClassic();
    cLog::log (LOGINFO, fmt::format ("imGui {} - {}", ImGui::GetVersion(), IMGUI_VERSION_NUM));

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    #if defined(BUILD_DOCKING)
      // enable Docking
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

      // enable multi-viewport / platform windows
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

      if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        // tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
    #endif

    ImGui_ImplSDL2_InitForOpenGL (mWindow, mGlContext);

    return true;
    }
  //}}}

  // gets
  //{{{
  virtual cPoint getWindowSize() final {

    int width = 0;
    int height = 0;
    //glfwGetWindowSize (mWindow, &width, &height);

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
      //mActionFullScreen = true;
      }
    }
  //}}}
  //{{{
  virtual void toggleFullScreen() final {

    mFullScreen = !mFullScreen;
    //mActionFullScreen = true;
    }
  //}}}

  // actions
  //{{{
  virtual bool pollEvents() final {

    //if (mActionFullScreen) {
    //  mActionFullScreen = false;
    //  }

    SDL_Event event;
    if (SDL_PollEvent (&event)) {
      ImGui_ImplSDL2_ProcessEvent (&event);
      if (event.type == SDL_QUIT)
        return false;
      if ((event.type == SDL_WINDOWEVENT) &&
          (event.window.event == SDL_WINDOWEVENT_CLOSE) &&
          (event.window.windowID == SDL_GetWindowID (mWindow)))
        return false;
      }

    return true;
    }
  //}}}
  virtual void newFrame() final { ImGui_ImplSDL2_NewFrame(); }
  //{{{
  virtual void present() final {

    #ifdef DOCKING
      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
        SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
        SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        SDL_GL_MakeCurrent (backup_current_window, backup_current_context);
        }
    #endif

    SDL_GL_SwapWindow (mWindow);
    }
  //}}}

private:
  //{{{
  void setSwapInterval (bool vsync) {
    if (vsync) {
      }
    else {
      }
    }
  //}}}

  SDL_Window* mWindow;
  SDL_GLContext mGlContext;
  };
//}}}

// cApp
//{{{
cApp::cApp (const string& name, const cPoint& windowSize, bool fullScreen, bool vsync) {

  // create platform
  cSdlPlatform* sdlPlatform = new cSdlPlatform (name);
  if (!sdlPlatform || !sdlPlatform->init (windowSize)) {
    cLog::log (LOGERROR, "cApp - sdlPlatform init failed");
    return;
    }

  // create graphics
  //mGraphics = new cGL3Graphics();
  //if (!mGraphics || !mGraphics->init()) {
  //  cLog::log (LOGERROR, "cApp - graphics init failed");
  //  return;
  //  }

  // set callbacks
  //glfwPlatform->mResizeCallback = [&](int width, int height) noexcept { windowResize (width, height); };
  //glfwPlatform->mDropCallback = [&](vector<string> dropItems) noexcept { drop (dropItems); };

  // fullScreen, vsync
  mPlatform = sdlPlatform;
  mPlatform->setFullScreen (fullScreen);
  mPlatform->setVsync (vsync);
  mPlatformDefined = true;
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

  cUI::render (*this);
  ImGui::Render();
  mGraphics->renderDrawData();

  mPlatform->present();
  }
//}}}

// main loop
//{{{
void cApp::mainUILoop() {

  while (mPlatform->pollEvents()) {
    mGraphics->newFrame();
    mPlatform->newFrame();
    ImGui::NewFrame();

    cUI::render (*this);
    ImGui::Render();
    mGraphics->renderDrawData();

    #if defined(BUILD_DOCKING)
      if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent (backupCurrentContext);
        }
    #endif

    mPlatform->present();
    }
  }
//}}}

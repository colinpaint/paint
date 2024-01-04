// imguiApp.cpp - imgui app
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

//{{{  include stb
// invoke header only library implementation here
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

  #define STB_IMAGE_IMPLEMENTATION
  #include <stb_image.h>
  #define STB_IMAGE_WRITE_IMPLEMENTATION
  #include <stb_image_write.h>

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}

// utils
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"
#include "../fmt/include/fmt/format.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../app/myImgui.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

using namespace std;
//}}}

//{{{
class cImguiApp : public cApp {
public:
  cImguiApp (cApp::cOptions* options, iUI* ui) : cApp ("imguiApp", options, ui) {}
  virtual ~cImguiApp() = default;

  virtual void drop (const vector<string>& dropItems) final {
    for (auto& item : dropItems) {
      string filename = cFileUtils::resolve (item);
      cLog::log (LOGINFO, filename);
      }
    }
  };
//}}}
//{{{
class cImguiUI : public cApp::iUI {
public:
  void virtual draw (cApp& app) final {

    bool show_demo_window = true;

    cGraphics& graphics = app.getGraphics();
    graphics.clear (cPoint((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

    if (app.getPlatform().hasFullScreen()) {
      ImGui::SameLine();
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      }

     if (app.getPlatform().hasVsync()) {
       // vsync button
       ImGui::SameLine();
       if (toggleButton ("vSync", app.getPlatform().getVsync()))
           app.getPlatform().toggleVsync();

       // fps text
       ImGui::SameLine();
       ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
       }

    ImGui::SameLine();
    ImGui::Text (fmt::format ("{}:{}",
                 ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());

    ImGui::ShowDemoWindow (&show_demo_window);
    }
  };
//}}}

int main (int numArgs, char* args[]) {

  // params
  cApp::cOptions* options = new cApp::cOptions();
  options->parse (numArgs, args);

  // log
  cLog::init (options->mLogLevel);
  cLog::log (LOGNOTICE, fmt::format ("fed"));

  // ImguiApp
  cImguiApp imguiApp (options, new cImguiUI());
  imguiApp.mainUILoop();

  return EXIT_SUCCESS;
  }

// demo.cpp - imgui demo app
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// utils
#include "common/date.h"
#include "common/utils.h"
#include "common/fileUtils.h"
#include "common/cLog.h"
#include "fmt/format.h"

// UI
#include "imgui/imgui.h"
#include "ui/cUI.h"
#include "font/itcSymbolBold.h"
#include "font/droidSansMono.h"
// app
#include "app/cApp.h"
#include "app/myImgui.h"
#include "app/cPlatform.h"
#include "app/cGraphics.h"

using namespace std;
//}}}

//{{{
class cDemoApp : public cApp {
public:
  cDemoApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp ("imgui demo", windowSize, fullScreen, vsync) {}
  virtual ~cDemoApp() = default;

  virtual void drop (const vector<string>& dropItems) final {
    for (auto& item : dropItems) {
      string filename = cFileUtils::resolve (item);
      cLog::log (LOGINFO, filename);
      }
    }
  };
//}}}
//{{{
class cDemoUI : public cUI {
public:
  cDemoUI (const string& name) : cUI(name) {}

  virtual ~cDemoUI() = default;

  void addToDrawList (cApp& app) final {

    bool show_demo_window = true;
    app.getGraphics().clear (cPoint((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

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

private:
  //{{{
  static cUI* create (const string& className) {
    return new cDemoUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("demo", &create);
  };
//}}}

int main (int numArgs, char* args[]) {

  // params
  eLogLevel logLevel = LOGINFO;
  bool fullScreen = false;
  bool vsync = true;
  //{{{  parse command line args to params
  // args to params
  vector <string> params;
  for (int i = 1; i < numArgs; i++)
    params.push_back (args[i]);

  // parse and remove recognised params
  for (auto it = params.begin(); it < params.end();) {
    if (*it == "log1") { logLevel = LOGINFO1; params.erase (it); }
    else if (*it == "log2") { logLevel = LOGINFO2; params.erase (it); }
    else if (*it == "log3") { logLevel = LOGINFO3; params.erase (it); }
    else if (*it == "full") { fullScreen = true; params.erase (it); }
    else if (*it == "free") { vsync = false; params.erase (it); }
    else ++it;
    };
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("fed"));

  // list static registered classes
  cUI::listRegisteredClasses();

  // app
  cDemoApp demoApp ({1000, 900}, fullScreen, vsync);
  //app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 16.f));
  //app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 16.f));

  #ifdef BUILD_IMPLOT
    ImPlot::CreateContext();
  #endif

  demoApp.mainUILoop();

  #ifdef BUILD_IMPLOT
    ImPlot::DestroyContext();
  #endif

  return EXIT_SUCCESS;
  }

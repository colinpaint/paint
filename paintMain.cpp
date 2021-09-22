// paintMain.cpp - paintbox main, uses imGui, stb
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

// imGui
#include "imgui/imgui.h"
#ifdef BUILD_IMPLOT
  #include "implot/implot.h"
#endif

// UI font
#include "font/itcSymbolBold.h"
#include "font/droidSansMono.h"

// self registered classes using static var init idiom
#include "platform/cPlatform.h"
#include "graphics/cGraphics.h"
#include "brush/cBrush.h"

#include "ui/cApp.h"
#include "ui/cUI.h"

// canvas
#include "canvas/cLayer.h"
#include "canvas/cCanvas.h"

// utils
#include "../utils/cFileUtils.h"
#include "utils/cLog.h"

using namespace std;
//}}}

int main (int numArgs, char* args[]) {

  // default params
  eLogLevel logLevel = LOGINFO;
  string platformName = "glfw";
  string graphicsName = "opengl";
  bool showDemoWindow = false;
  bool fullScreen = false;
  bool vsync = true;
  bool showPlotWindow = false;
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
    else if (*it == "dx11") { platformName = "win32"; graphicsName = "dx11"; params.erase (it); }
    else if (*it == "demo") { showDemoWindow = true; params.erase (it); }
    else if (*it == "full") { fullScreen = true; params.erase (it); }
    else if (*it == "free") { vsync = false; params.erase (it); }
    else if (*it == "plot") { showPlotWindow = true; params.erase (it); }
    else ++it;
    };
  //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("paintbox {} {}", platformName, graphicsName));

  // list static registered classes
  cPlatform::listRegisteredClasses();
  cGraphics::listRegisteredClasses();
  cBrush::listRegisteredClasses();
  cUI::listRegisteredClasses();

  // create platform, graphics, UI font
  cPlatform& platform = cPlatform::createByName (platformName, cPoint(1200, 800), false, vsync, fullScreen);
  cGraphics& graphics = cGraphics::createByName (graphicsName, platform);
  //{{{  create canvas and our fonts
  #ifdef _WIN32
    cCanvas canvas (platform, graphics,
      params.empty() ? "../piccies/tv.jpg" : cFileUtils::resolveShortcut (params[0]));
  #else
    cCanvas canvas (platform, graphics, params.empty() ? "../piccies/tv.jpg" : params[0]);
  #endif

  canvas.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
  canvas.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(&droidSansMono, droidSansMonoSize, 16.f));
  //}}}
  if (params.size() > 1)
    canvas.newLayer (params[1]);

  platform.setResizeCallback (
    //{{{  resize lambda
    [&](int width, int height) noexcept {
      platform.newFrame();
      graphics.windowResize (width, height);
      graphics.newFrame();
      if (showDemoWindow)
        cUI::draw (canvas);
      ImGui::ShowDemoWindow (&showDemoWindow);
      graphics.drawUI (platform.getWindowSize());
      platform.present();
      }
    );
    //}}}
  platform.setDropCallback (
    //{{{  drop lambda
    [&](vector<string> dropItems) noexcept {

      for (auto& item : dropItems) {
        cLog::log (LOGINFO, item);
        canvas.newLayer (item);
        }
      }
    );
    //}}}

  // main UI loop
  while (platform.pollEvents()) {
    platform.newFrame();
    graphics.newFrame();
    cUI::draw (canvas);
    if (showDemoWindow)
      ImGui::ShowDemoWindow (&showDemoWindow);
    if (showPlotWindow) {
      //{{{  optional implot
      #ifdef BUILD_IMPLOT
        ImPlot::ShowDemoWindow();
      #endif
      }
      //}}}

    graphics.drawUI (platform.getWindowSize());
    platform.present();
    }

  // cleanup
  graphics.shutdown();
  platform.shutdown();

  return EXIT_SUCCESS;
  }

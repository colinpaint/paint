// main.cpp - paintbox main, uses imGui, stb
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
//#include <format>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// imGui
#include <imgui.h>
#include "../implot/implot.h"

// UI font
#include "font/itcSymbolBold.h"

// self registered classes using static var init idiom
#include "platform/cPlatform.h"
#include "graphics/cGraphics.h"
#include "brush/cBrush.h"
#include "ui/cUI.h"

// canvas
#include "canvas/cLayer.h"
#include "canvas/cCanvas.h"

// log
#include "utils/cLog.h"

using namespace std;
//}}}

int main (int numArgs, char* args[]) {

  // default params
  eLogLevel logLevel = LOGINFO;
  string platformName = "glfw";
  string graphicsName = "opengl";
  bool showDemoWindow = false;
  bool vsync = false;
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
    else if (*it == "vsync") { vsync = true; params.erase (it); }
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
  cPlatform& platform = cPlatform::createByName (platformName, cPoint(1200, 800), false, vsync);
  cGraphics& graphics = cGraphics::createByName (graphicsName, platform);
  ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f);

  // should clear main screen here before file loads

  // create canvas
  cCanvas canvas (params.empty() ? "../piccies/tv.jpg" : params[0], graphics);
  if (params.size() > 1)
    canvas.newLayer (params[1]);

  platform.setResizeCallback (
    //{{{  resize lambda
    [&](int width, int height) noexcept {
      platform.newFrame();
      graphics.windowResize (width, height);
      graphics.newFrame();
      if (showDemoWindow)
        cUI::draw (canvas, graphics, platform);
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
    cUI::draw (canvas, graphics, platform);
    if (showDemoWindow)
      ImGui::ShowDemoWindow (&showDemoWindow);
    if (showPlotWindow) {
      //{{{  optional implot
      #ifdef USE_IMPLOT
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

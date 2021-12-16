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
#include "../imgui/imgui.h"
#ifdef BUILD_IMPLOT
  #include "../implot/implot.h"
#endif

#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// self registered classes using static var init idiom
#include "../brush/cBrush.h"

#include "../ui/cUI.h"

// canvas
#include "cLayer.h"
#include "cPaintApp.h"

// utils
#include "../utils/cFileUtils.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

int main (int numArgs, char* args[]) {

  // default params
  eLogLevel logLevel = LOGINFO;
  bool showDemoWindow = false;
  bool fullScreen = false;
  bool vsync = true;
  string filename;
  //{{{  parse args

  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "log1") { logLevel = LOGINFO1; }
    else if (param == "log2") { logLevel = LOGINFO2; }
    else if (param == "log3") { logLevel = LOGINFO3;  }
    else if (param == "demo") { showDemoWindow = true;  }
    else if (param == "full") { fullScreen = true;  }
    else if (param == "free") { vsync = false; }
    else filename = param;
    }
  //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("paintbox"));

  // list static registered classes
  cBrush::listRegisteredClasses();
  cUI::listRegisteredClasses();

  cPaintApp app ({1200, 800}, filename.empty() ? "../piccies/tv.jpg" : cFileUtils::resolve (filename));
  app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
  app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(&droidSansMono, droidSansMonoSize, 16.f));

  cPlatform& platform = app.getPlatform();
  cGraphics& graphics = app.getGraphics();
  platform.setVsync (vsync);
  platform.setFullScreen (fullScreen);
  platform.setResizeCallback (
    //{{{  resize lambda
    [&](int width, int height) noexcept {
      platform.newFrame();
      graphics.windowResize (width, height);
      graphics.newFrame();
      if (showDemoWindow)
        cUI::draw (app);
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
        app.newLayer (cFileUtils::resolve (item));
        }
      }
    );
    //}}}

  // main UI loop
  while (platform.pollEvents()) {
    platform.newFrame();
    graphics.newFrame();
    cUI::draw (app);
    //ImGui::ShowDemoWindow (&showDemoWindow);
    graphics.drawUI (platform.getWindowSize());
    app.getPlatform().present();
    }

  return EXIT_SUCCESS;
  }

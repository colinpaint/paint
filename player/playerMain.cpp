// playerMain.cpp - imgui player main
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

// UI font
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"
#include"cPlayerApp.h"

// ui
#include "../ui/cUI.h"

// utils
#include "../utils/cLog.h"
#include "../utils/cFileUtils.h"

using namespace std;
//}}}

int main (int numArgs, char* args[]) {

  // default params
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

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("player"));

  // list static registered classes
  cUI::listRegisteredClasses();

  cPlayerApp app (cPoint(800, 480));
  app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 20.f));
  app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 20.f));
  app.setSongName (params.empty() ? "" : cFileUtils::resolve (params[0]));

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
      cUI::draw (app);
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
        app.setSongName (cFileUtils::resolve (item));
        }
      }
    );
    //}}}

  // main UI loop
  while (platform.pollEvents()) {
    platform.newFrame();
    graphics.newFrame();
    cUI::draw (app);
    graphics.drawUI (platform.getWindowSize());
    platform.present();
    }

  return EXIT_SUCCESS;
  }

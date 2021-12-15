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
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// self registered classes using static var init idiom
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"

// ui
#include"cPlayerApp.h"
#include "../ui/cUI.h"

// utils
#include "../utils/cLog.h"
#include "../utils/cFileUtils.h"

using namespace std;
//}}}

int main (int numArgs, char* args[]) {

  // default params
  eLogLevel logLevel = LOGINFO;
  string platformName = "glfw";
  string graphicsName = "opengl";
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
    else if (*it == "dx11") { platformName = "win32"; graphicsName = "dx11"; params.erase (it); }
    else if (*it == "full") { fullScreen = true; params.erase (it); }
    else if (*it == "free") { vsync = false; params.erase (it); }
    else ++it;
    };
  //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("player {} {}", platformName, graphicsName));

  // list static registered classes
  cPlatform::listRegisteredClasses();
  cGraphics::listRegisteredClasses();
  cUI::listRegisteredClasses();

  // create platform, graphics, UI font
  cPlatform& platform = cPlatform::createByName (platformName, cPoint(800, 480), false, vsync, fullScreen);
  cGraphics& graphics = cGraphics::createByName (graphicsName, platform);
  ImFont* mainFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 20.f);
  ImFont* monoFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 20.f);
  cPlayerApp app (platform, graphics, mainFont, monoFont);
  app.setSongName (params.empty() ? "" : cFileUtils::resolve (params[0]));

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

  // cleanup
  graphics.shutdown();
  platform.shutdown();

  return EXIT_SUCCESS;
  }

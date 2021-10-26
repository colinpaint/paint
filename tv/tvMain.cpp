// tvMain.cpp - imgui tv dvb analyser,splitter main
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
#include "../platform/cPlatform.h"
#include "../graphics/cGraphics.h"

// ui
#include "../utils/cApp.h"
#include "../ui/cUI.h"

// utils
#include "../utils/cFileUtils.h"
#include "../utils/cLog.h"

#include "../tv/cTvApp.h"

using namespace std;
//}}}
//{{{
static const vector <cDvbMultiplex> kDvbMultiplexes = {
  { "hd",
    626000000,
    { "BBC ONE HD", "BBC TWO HD", "ITV HD", "Channel 4 HD", "Channel 5 HD" },
    { "bbc1hd",     "bbc2hd",     "itv1hd", "chn4hd",       "chn5hd" },
    false,
  },

  { "itv",
    650000000,
    { "ITV",  "ITV2", "ITV3", "ITV4", "Channel 4", "Channel 4+1", "More 4", "Film4" , "E4", "Channel 5" },
    { "itv1", "itv2", "itv3", "itv4", "chn4"     , "c4+1",        "more4",  "film4",  "e4", "chn5" },
    false,
  },

  { "bbc",
    674000000,
    { "BBC ONE S West", "BBC TWO", "BBC FOUR" },
    { "bbc1",           "bbc2",    "bbc4" },
    false,
  },

  { "allhd",
    626000000,
    { "" },
    { "" },
    true,
  },

  { "allitv",
    650000000,
    { "" },
    { "" },
    true,
  },

  { "allbbc",
    674000000,
    { "" },
    { "" },
    true,
  },
  };
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

  cDvbMultiplex useMultiplex = kDvbMultiplexes[0];
  string filename;

  // parse params
  for (auto& param : params) {
    if (param == "log1") { logLevel = LOGINFO1; }
    else if (param == "log2") { logLevel = LOGINFO2; }
    else if (param == "log3") { logLevel = LOGINFO3; }
    else if (param == "dx11") { platformName = "win32"; graphicsName = "dx11"; }
    else if (param == "full") { fullScreen = true; }
    else if (param == "free") { vsync = false; }
    else {
      // assume param is filename unless it matches multiplex name
      filename = param;
      bool found = false;
      for (auto& multiplex : kDvbMultiplexes)
        if (param == multiplex.mName) {
          useMultiplex = multiplex;
          found = true;
          filename = "";
          break;
          }
      }
    }
  //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("telly {} {}", platformName, graphicsName));
  if (filename.empty())
    cLog::log (LOGINFO, fmt::format ("using multiplex {}", useMultiplex.mName));

  // list static registered classes
  cPlatform::listRegisteredClasses();
  cGraphics::listRegisteredClasses();
  cUI::listRegisteredClasses();

  // create platform, graphics, UI fonts
  cPlatform& platform = cPlatform::createByName (platformName, cPoint(800, 600), false, vsync, fullScreen);
  cGraphics& graphics = cGraphics::createByName (graphicsName, platform);
  ImFont* mainFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f);
  ImFont* monoFont = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 18.f);
  cTvApp app (platform, graphics, mainFont, monoFont);
  app.setDvbSource (cFileUtils::resolve (filename), useMultiplex);

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
        app.setDvbSource (item, useMultiplex);
        cLog::log(LOGINFO, item);
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

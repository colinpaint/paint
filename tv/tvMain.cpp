// tvMain.cpp - imgui player main
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <thread>

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
#include "../ui/cApp.h"
#include "../ui/cUI.h"

// utils
#include "../utils/cFileUtils.h"
#include "../utils/cLog.h"

// dvb
#include "../dvb/cTsDvb.h"

using namespace std;
//}}}
//{{{  const multiplexes
struct sMultiplex {
  string mName;
  int mFrequency;
  vector <string> mSelectedChannels;
  vector <string> mSaveNames;
  };

const sMultiplex kHdMultiplex = {
  "hd",
  626000000,
  { "BBC ONE HD", "BBC TWO HD", "ITV HD", "Channel 4 HD", "Channel 5 HD" },
  { "bbc1hd",     "bbc2hd",     "itv1hd", "chn4hd",       "chn5hd" }
  };

const sMultiplex kItvMultiplex = {
  "itv",
  650000000,
  { "ITV",  "ITV2", "ITV3", "ITV4", "Channel 4", "Channel 4+1", "More 4", "Film4" , "E4", "Channel 5" },
  { "itv1", "itv2", "itv3", "itv4", "chn4"     , "c4+1",        "more4",  "film4",  "e4", "chn5" }
  };

const sMultiplex kBbcMultiplex = {
  "bbc",
  674000000,
  { "BBC ONE S West", "BBC TWO", "BBC FOUR" },
  { "bbc1",           "bbc2",    "bbc4" }
  };

const sMultiplex kAllMultiplex = {
  "all",
  0,
  { "all" },
  { "" }
  };

struct sMultiplexes {
  vector <sMultiplex> mMultiplexes;
  };
const sMultiplexes kMultiplexes = { { kHdMultiplex, kItvMultiplex, kBbcMultiplex } };

#ifdef _WIN32
  const string kRootName = "/tv";
#else
  const string kRootName = "/home/pi/tv";
#endif
//}}}

int main (int numArgs, char* args[]) {

  // default params
  eLogLevel logLevel = LOGINFO;
  string platformName = "glfw";
  string graphicsName = "opengl";
  bool fullScreen = false;
  bool vsync = true;
  bool all = false;
  bool decodeSubtitle = false;
  bool gui = true;
  sMultiplex multiplex = kHdMultiplex;
  //{{{  parse command line args to params
  // args to params
  vector <string> params;
  for (int i = 1; i < numArgs; i++)
    params.push_back (args[i]);

  // parse and remove recognised params
  for (auto it = params.begin(); it < params.end();) {
    // look for named multiplex
    bool multiplexFound = false;
    for (size_t j = 0; j < kMultiplexes.mMultiplexes.size() && !multiplexFound; j++) {
      if (*it == kMultiplexes.mMultiplexes[j].mName) {
        multiplex = kMultiplexes.mMultiplexes[j];
        multiplexFound = true;
        }
      }
    if (multiplexFound) {
      ++it;
      continue;
      }

    if (*it == "log1") { logLevel = LOGINFO1; params.erase (it); }
    else if (*it == "log2") { logLevel = LOGINFO2; params.erase (it); }
    else if (*it == "log3") { logLevel = LOGINFO3; params.erase (it); }
    else if (*it == "dx11") { platformName = "win32"; graphicsName = "dx11"; params.erase (it); }
    else if (*it == "full") { fullScreen = true; params.erase (it); }
    else if (*it == "free") { vsync = false; params.erase (it); }
    else if (*it == "gui") { gui = false; params.erase (it); }
    else if (*it == "sub") { decodeSubtitle = true; params.erase (it); }
    else if (*it == "all") { all = true; params.erase (it); }
    else ++it;
    };
  //}}}

  // start log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("tv {} {}", platformName, graphicsName));

  // list static registered classes
  cPlatform::listRegisteredClasses();
  cGraphics::listRegisteredClasses();
  cUI::listRegisteredClasses();

  // create platform, graphics, UI font
  cPlatform& platform = cPlatform::createByName (platformName, cPoint(800, 600), false, vsync, fullScreen);
  cGraphics& graphics = cGraphics::createByName (graphicsName, platform);

  // create app to tie stuff together
  cApp app (platform, graphics);
  app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));

  #ifdef _WIN32
    app.setName (params.empty() ? "" : cFileUtils::resolveShortcut (params[0]));
  #else
    app.setName (params.empty() ? "" : params[0]);
  #endif

  // add monoSpaced font
  app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 18.f));

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
      for (auto& item : dropItems)
        cLog::log (LOGINFO, item);
      }
    );
    //}}}

  // create dvb
  auto mDvb = new cTsDvb (multiplex.mFrequency, kRootName, multiplex.mSelectedChannels, multiplex.mSaveNames,
                          decodeSubtitle);
  string fileName = params.empty() ? "" : cFileUtils::resolveShortcut(params[0]);
  if (fileName.empty()) {
    //{{{  grab from dvb card
    if (gui)
      thread ([=](){ mDvb->grabThread (all ? kRootName : "", multiplex.mName); } ).detach();
    else
      mDvb->grabThread (all ? kRootName : "", multiplex.mName);
    }
    //}}}
  else {
    //{{{  read from file
    if (gui)
      thread ([=](){ mDvb->readThread (fileName); } ).detach();
    else
      mDvb->readThread (fileName);
    }
    //}}}

  if (gui) {
    // main UI loop
    while (platform.pollEvents()) {
      platform.newFrame();
      graphics.newFrame();
      cUI::draw (app);
      graphics.drawUI (platform.getWindowSize());
      platform.present();
      }
    }
  else {
    while (platform.pollEvents()) {
      this_thread::sleep_for (40ms);
      }
    }

  delete mDvb;

  // cleanup
  graphics.shutdown();
  platform.shutdown();

  return EXIT_SUCCESS;
  }

// tellyMain.cpp - imgui telly main
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// imGui
#include "../imgui/imgui.h"
#include "../implot/implot.h"

// app
#include "cTellyApp.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

#include "../ui/cUI.h"

// utils
#include "../common/cFileUtils.h"
#include "../common/cLog.h"


using namespace std;
//}}}

namespace {
  #ifdef _WIN32
    const string kRecordRoot = "/tv/";
  #else
    const string kRecordRoot = "/home/pi/tv/";
  #endif
  //{{{
  const vector <cDvbMultiplex> kDvbMultiplexes = {
    { "hd",
      626000000,
      { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
      { "bbc1hd",        "bbc2hd",     "bbc3hd",       "bbc4hd",      "itv1hd",  "chn4hd",       "chn5hd" },
      false,
    },

    { "itv",
      650000000,
      { "ITV1",  "ITV2", "ITV3", "ITV4", "Channel 4", "Channel 4+1", "More 4", "Film4" , "E4", "Channel 5" },
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
  }

int main (int numArgs, char* args[]) {

  // params
  eLogLevel logLevel = LOGINFO;
  uint16_t decoderOptions = 0;
  bool renderFirstService = false;
  bool fullScreen = false;
  bool vsync = true;

  //{{{  parse command line args to params
  cDvbMultiplex useMultiplex = kDvbMultiplexes[0];
  string filename;

  // parse params
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "log1") { logLevel = LOGINFO1; }
    else if (param == "log2") { logLevel = LOGINFO2; }
    else if (param == "log3") { logLevel = LOGINFO3; }
    else if (param == "full") { fullScreen = true; }
    else if (param == "free") { vsync = false; }
    else if (param == "first") { renderFirstService = true; }
    else if (param == "mfx") { decoderOptions = 1; }
    else if (param == "mfxvid") { decoderOptions = 2; }
    else {
      // assume param is filename unless it matches multiplex name
      filename = param;
      for (auto& multiplex : kDvbMultiplexes)
        if (param == multiplex.mName) {
          useMultiplex = multiplex;
          filename = "";
          break;
          }
      }
    }
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("telly"));
  if (filename.empty())
    cLog::log (LOGINFO, fmt::format ("using multiplex {}", useMultiplex.mName));

  // list static registered classes
  cUI::listRegisteredClasses();

  // app
  cTellyApp app ({1920/2, 1080/2}, fullScreen, vsync);
  app.setDvbSource (cFileUtils::resolve (filename), kRecordRoot, useMultiplex,
                    !filename.empty() || renderFirstService, decoderOptions);
  app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
  app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 18.f));

  ImPlot::CreateContext();
  app.mainUILoop();
  ImPlot::DestroyContext();

  return EXIT_SUCCESS;
  }

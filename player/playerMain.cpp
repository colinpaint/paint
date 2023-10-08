// playerMain.cpp - imgui player main
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>
#include <vector>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// imGui
#include "../imgui/imgui.h"

// UI font
#include"cPlayerApp.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// ui
#include "../ui/cUI.h"

// utils
#include "../common/cLog.h"
#include "../common/fileUtils.h"

using namespace std;
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
  cLog::log (LOGNOTICE, fmt::format ("player"));

  // list static registered classes
  cUI::listRegisteredClasses();

  // app
  cPlayerApp app ({800, 480}, fullScreen, vsync);
  app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 20.f));
  app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 20.f));
  app.setSongName (params.empty() ? "" : cFileUtils::resolve (params[0]));

  app.mainUILoop();

  return EXIT_SUCCESS;
  }

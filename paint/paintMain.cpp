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

#include "cPaintApp.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// self registered classes using static var init idiom
#include "../brush/cBrush.h"
#include "../ui/cUI.h"

// utils
#include "../utils/cFileUtils.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

int main (int numArgs, char* args[]) {

  // params
  eLogLevel logLevel = LOGINFO;
  bool fullScreen = false;
  bool vsync = true;
  string filename;
  //{{{  parse args

  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "log1") { logLevel = LOGINFO1; }
    else if (param == "log2") { logLevel = LOGINFO2; }
    else if (param == "log3") { logLevel = LOGINFO3;  }
    else if (param == "full") { fullScreen = true;  }
    else if (param == "free") { vsync = false; }
    else filename = param;
    }
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("paintbox"));

  // list static registered classes
  cBrush::listRegisteredClasses();
  cUI::listRegisteredClasses();

  // app
  cPaintApp app ({1200, 800}, fullScreen, vsync,
                 filename.empty() ? "../piccies/tv.jpg" : cFileUtils::resolve (filename));
  app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
  app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(&droidSansMono, droidSansMonoSize, 16.f));

  app.mainUILoop();

  return EXIT_SUCCESS;
  }

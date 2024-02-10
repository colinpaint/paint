// telly.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>

// utils
#include "../date/include/date/date.h"
#include "../common/basicTypes.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

#include "cTellyApp.h"
#include "cTellyUI.h"

// app
#include "../app/cPlatform.h"
#include "../app/myImgui.h"

using namespace std;
//}}}
namespace {
  //{{{  const string kRootDir
    #ifdef _WIN32
      const string kRootDir = "/tv/";
    #else
      const string kRootDir = "/home/pi/tv/";
    #endif
    //}}}
  }

// main
int main (int numArgs, char* args[]) {

  cTellyOptions* options = new cTellyOptions();
  //{{{  parse commandLine params to options
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) {
      // found a cApp option
      }
    else if (param == "all")
      options->mRecordAll = true;
    else if (param == "head") {
      options->mHasGui = false;
      options->mShowAllServices = false;
      options->mShowFirstService = false;
      }
    else if (param == "single") {
      options->mShowAllServices = false;
      options->mShowFirstService = true;
      }
    else if (param == "noaudio")
      options->mHasAudio = false;
    else if (param == "sub")
      options->mShowSubtitle = true;
    else if (param == "motion")
      options->mHasMotionVectors = true;
    else {
      // assume filename
      options->mFileName = param;

      // look for named multiplex
      for (auto& multiplex : kDvbMultiplexes) {
        if (param == multiplex.mName) {
          // found named multiplex
          options->mMultiplex = multiplex;
          options->mFileName = "";
          break;
          }
        }
      }
    }
  //}}}

  // log
  cLog::init (options->mLogLevel);
  cLog::log (LOGNOTICE, fmt::format ("telly head all single noaudio {} {}",
                                     (dynamic_cast<cApp::cOptions*>(options))->cApp::cOptions::getString(),
                                     options->cTellyOptions::getString()));
  if (options->mFileName.empty()) {
    //  create cTellyApp,cTellyUI with liveDvb source 
    options->mIsLive = true;
    options->mRecordRoot = kRootDir;
    cTellyApp tellyApp (options, new cTellyUI());
    tellyApp.liveDvbSource (options->mMultiplex, options);
    tellyApp.mainUILoop();
    }
  else {
    // create cTellyApp,cTellyUI with file source
    cTellyApp tellyApp (options, new cTellyUI());
    tellyApp.fileSource (options->mFileName, options);
    tellyApp.mainUILoop();
    }

  return EXIT_SUCCESS;
  }

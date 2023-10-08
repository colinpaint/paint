// cDemoApp.cpp - fedApp info holder from fedMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cDemoApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/fileUtils.h"

using namespace std;
//}}}

cDemoApp::cDemoApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp ("imgui demo", windowSize, fullScreen, vsync) {
  }


void cDemoApp::drop (const vector<string>& dropItems) {
  for (auto& item : dropItems) {
    string filename = cFileUtils::resolve (item);
    cLog::log (LOGINFO, filename);
    }
  }

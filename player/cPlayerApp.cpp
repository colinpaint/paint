// cPlayerApp.cpp - playerApp info holder from playerMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cPlayerApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/date.h"
#include "../utils/cLog.h"

#include "../dvb/cDvbTransportStream.h"

using namespace std;
//}}}

bool cPlayerApp::setSource (const string& sourceName) {
// create dvb source

  mSourceName = sourceName;

  return true;
  }

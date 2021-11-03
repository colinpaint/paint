// cTvApp.cpp - tvApp info holder from tvMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cTvApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/date.h"
#include "../utils/cLog.h"

#include "../dvb/cDvbStream.h"

using namespace std;
//}}}

bool cTvApp::setDvbSource (const string& filename, const cDvbMultiplex& dvbMultiplex) {
// create dvb source

  #ifdef _WIN32
    const string kRecordRoot = "/tv/";
  #else
    const string kRecordRoot = "/home/pi/tv/";
  #endif

  mDvbStream = new cDvbStream (dvbMultiplex, kRecordRoot);
  if (!mDvbStream)
    return false;

  if (filename.empty())
    mDvbStream->dvbSource (true);
  else
    mDvbStream->fileSource (true, filename);

  return true;
  }

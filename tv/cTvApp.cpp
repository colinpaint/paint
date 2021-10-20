// cTvApp.cpp - tvApp info holder from tvMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cTvApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/date.h"
#include "../utils/cLog.h"

#include "../dvb/cDvbTransportStream.h"

using namespace std;
//}}}

bool cTvApp::setDvbSource (const string& filename, const cDvbMultiplex& dvbMultiplex, bool subtitle) {
// create dvb source

  mSubtitle = subtitle;

  #ifdef _WIN32
    const string kRecordRoot = "/tv/";
  #else
    const string kRecordRoot = "/home/pi/tv/";
  #endif

  mDvbTransportStream = new cDvbTransportStream (dvbMultiplex, kRecordRoot, subtitle);
  if (!mDvbTransportStream)
    return false;

  if (filename.empty())
    mDvbTransportStream->dvbSource (true);
  else
    mDvbTransportStream->fileSource (true, filename);

  return true;
  }

void cTvApp::toggleSubtitle() { 
  mSubtitle = !mSubtitle; 
  mDvbTransportStream->setSubtitle (mSubtitle);
  }
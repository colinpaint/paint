// cTellyApp.cpp - tvApp info holder from tvMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cTellyApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/cFileUtils.h"

#include "../dvb/cDvbStream.h"

using namespace std;
//}}}

cTellyApp::cTellyApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp(windowSize, fullScreen, vsync) {
  }

bool cTellyApp::setDvbSource (const string& filename, const string& recordRoot, const cDvbMultiplex& dvbMultiplex,
                              bool renderFirstService, uint16_t decoderOptions) {
// create dvb source

  mDecoderOptions = decoderOptions;
  mDvbStream = new cDvbStream (dvbMultiplex, recordRoot, renderFirstService, decoderOptions);
  if (!mDvbStream)
    return false;

  if (filename.empty())
    mDvbStream->dvbSource (true);
  else
    mDvbStream->fileSource (true, filename);

  return true;
  }

void cTellyApp::drop (const vector<string>& dropItems) {

  // wrong !!!!
  const cDvbMultiplex kDvbMultiplexes =
      { "hd",
        626000000,
        { "BBC ONE HD", "BBC TWO HD", "ITV HD", "Channel 4 HD", "Channel 5 HD" },
        { "bbc1hd",     "bbc2hd",     "itv1hd", "chn4hd",       "chn5hd" },
        false,
      };

  for (auto& item : dropItems) {
    string filename = cFileUtils::resolve (item);
    setDvbSource (filename, "", kDvbMultiplexes, true, mDecoderOptions);
    cLog::log (LOGINFO, filename);
    }
  }

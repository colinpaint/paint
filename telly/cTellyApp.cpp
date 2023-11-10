// cTellyApp.cpp - tvApp info holder from tvMain
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include "cTellyApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/fileUtils.h"

#include "fmt/format.h"

#include "../dvb/cDvbStream.h"

using namespace std;
//}}}

namespace {
  // wrong !!!!
  const cDvbMultiplex kDvbMultiplexes =
      { "hd",
        0,
        { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
        { "bbc1hd",        "bbc2hd",     "bbc3hd",       "bbc4hd",      "itv1hd",  "chn4hd",       "chn5hd" },
        false,
      };
  }

//{{{
cTellyApp::cTellyApp (const cPoint& windowSize, bool fullScreen, bool vsync)
  : cApp("telly", windowSize, fullScreen, vsync) {}
//}}}

//{{{
bool cTellyApp::setDvbSource (const string& filename, const string& recordRoot, const cDvbMultiplex& dvbMultiplex,
                              bool renderFirstService, uint16_t decoderOptions) {

  mDecoderOptions = decoderOptions;

  if (filename.empty()) {
  // create dvbSource from dvbMultiplex
    mDvbStream = new cDvbStream (dvbMultiplex, recordRoot, renderFirstService, mDecoderOptions);
    if (!mDvbStream)
      return false;
    mDvbStream->dvbSource (true);
    return true;
    }

  else {
    // create fileSource, with dummy all multiplex
    mDvbStream = new cDvbStream (kDvbMultiplexes,  "", true, mDecoderOptions);
    if (!mDvbStream)
      return false;

    cFileUtils::resolve (filename);
    mDvbStream->fileSource (true, filename);

    return true;
    }
  }
//}}}
//{{{
void cTellyApp::drop (const vector<string>& dropItems) {
// drop fileSource

  for (auto& item : dropItems) {
    string filename = cFileUtils::resolve (item);
    setDvbSource (filename, "", kDvbMultiplexes, true, mDecoderOptions);
    cLog::log (LOGINFO, fmt::format ("cTellyApp::drop {}", filename));
    }
  }
//}}}

// cTvApp - tvApp info holder from tvMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../ui/cApp.h"
#include "../dvb/cDvbTransportStream.h"

class cPlatform;
class cGraphics;
//}}}

class cTvApp : public cApp {
public:
  cTvApp (cPlatform& platform, cGraphics& graphics, const cDvbMultiplex& dvbMultiplex, bool subtitles)
    : cApp (platform, graphics), mDvbMultiplex(dvbMultiplex), mSubtitles(subtitles) {}
  ~cTvApp() = default;

  cDvbMultiplex getDvbMultiplex() { return mDvbMultiplex; }
  bool getSubtitles() { return mSubtitles; }

private:
  const cDvbMultiplex mDvbMultiplex;
  bool mSubtitles = false;
  };

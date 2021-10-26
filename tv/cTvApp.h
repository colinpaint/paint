// cTvApp.h - tvApp info holder from tvMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/cApp.h"
#include "../dvb/cDvbMultiplex.h"

class cPlatform;
class cGraphics;
class cDvbTransportStream;
//}}}

class cTvApp : public cApp {
public:
  cTvApp (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont)
    : cApp (platform, graphics, mainFont, monoFont) {}
  ~cTvApp() = default;

  cDvbTransportStream* getDvbTransportStream() { return mDvbTransportStream; }

  bool setDvbSource (const std::string& filename, const cDvbMultiplex& dvbMultiplex);

private:
  cDvbTransportStream* mDvbTransportStream = nullptr;
  };

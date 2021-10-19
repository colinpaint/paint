// cTvApp.h - tvApp info holder from tvMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../ui/cApp.h"
#include "../dvb/cDvbMultiplex.h"

class cPlatform;
class cGraphics;
class cDvbTransportStream;
//}}}

class cTvApp : public cApp {
public:
  cTvApp (cPlatform& platform, cGraphics& graphics) : cApp (platform, graphics) {}
  ~cTvApp() = default;

  cDvbTransportStream* getDvbTransportStream() { return mDvbTransportStream; }
  bool getSubtitle() const { return mSubtitle; }

  bool setDvbSource (const std::string& filename, const cDvbMultiplex& dvbMultiplex, bool subtitle);
  void toggleSubtitle();

private:
  cDvbTransportStream* mDvbTransportStream = nullptr;

  bool mSubtitle = false;
  };

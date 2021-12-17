// cTellyApp.h - tvApp info holder from tvMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../app/cApp.h"
#include "../dvb/cDvbMultiplex.h"

class cPlatform;
class cGraphics;
class cDvbStream;
//}}}

class cTellyApp : public cApp {
public:
  cTellyApp (const cPoint& windowSize, bool fullScreen, bool vsync);
  virtual ~cTellyApp() = default;

  cDvbStream* getDvbStream() { return mDvbStream; }
  uint16_t getDecoderOptions() { return mDecoderOptions; }

  bool setDvbSource (const std::string& filename, const std::string& recordRoot, const cDvbMultiplex& dvbMultiplex,
                     bool renderFirstService, uint16_t decoderOptions);

  virtual void drop (const std::vector<std::string>& dropItems) final;

private:
  cDvbStream* mDvbStream = nullptr;
  uint16_t mDecoderOptions = 0;
  };

// cTvApp - tvApp info holder from tvMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../ui/cApp.h"

class cPlatform;
class cGraphics;
class cTsDvb;
//}}}

struct sMultiplex {
  std::string mName;
  int mFrequency;
  std::vector <std::string> mSelectedChannels;
  std::vector <std::string> mSaveNames;
  };

struct sMultiplexes {
  std::vector <sMultiplex> mMultiplexes;
  };

class cTvApp : public cApp {
public:
  cTvApp (cPlatform& platform, cGraphics& graphics, sMultiplex& multiplex, bool subtitles)
    : cApp (platform, graphics), mMultiplex(multiplex), mSubtitles(subtitles) {}
  ~cTvApp() = default;

  sMultiplex getMultiplex() { return mMultiplex; }
  bool getSubtitles() { return mSubtitles; }

private:
  sMultiplex mMultiplex;
  bool mSubtitles = false;
  };

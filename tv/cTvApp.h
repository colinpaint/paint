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

class cTvApp : public cApp {
public:
  cTvApp (cPlatform& platform, cGraphics& graphics, cTsDvb& tsDvb)
    : cApp (platform, graphics), mTsDvb(tsDvb) {}
  ~cTvApp() = default;

  cTsDvb& getTsDvb() { return mTsDvb; }

private:
  cTsDvb& mTsDvb;
  };

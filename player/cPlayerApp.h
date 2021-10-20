// cPlayerApp.h - playerApp info holder from playerMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../ui/cApp.h"
class cPlatform;
class cGraphics;
//}}}

class cPlayerApp : public cApp {
public:
  cPlayerApp (cPlatform& platform, cGraphics& graphics) : cApp (platform, graphics) {}
  ~cPlayerApp() = default;

  std::string getSourceName() const { return mSourceName; }

  bool setSource (const std::string& sourceName);

private:
  std::string mSourceName;
  };

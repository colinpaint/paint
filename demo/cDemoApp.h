// cDemoApp.h - playerApp info holder from playerMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../app/cApp.h"

class cPlatform;
class cGraphics;
class cDocument;
//}}}

class cDemoApp : public cApp {
public:
  cDemoApp (const cPoint& windowSize, bool fullScreen, bool vsync);
  virtual ~cDemoApp() = default;

  virtual void drop (const std::vector<std::string>& dropItems) final;
  };

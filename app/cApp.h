// cApp.h - base class - application context, platform,graphics interface, platform,graphics independent stuff
#pragma once
//{{{  includes
#include <string>
#include <chrono>

#include "../utils/cPointRectColor.h"

class cPlatform;
class cGraphics;
struct ImFont;
//}}}

class cApp {
public:
  cApp (const cPoint& windowSize, bool fullScreen, bool vsync);
  virtual ~cApp();

  // get interfaces
  cPlatform& getPlatform() const { return *mPlatform; }
  cGraphics& getGraphics() const { return *mGraphics; }

  // time of day
  std::chrono::system_clock::time_point getNow();

  // fonts
  ImFont* getMainFont() const { return mMainFont; }
  ImFont* getMonoFont() const { return mMonoFont; }

  // sets
  void setMainFont (ImFont* font) { mMainFont = font; }
  void setMonoFont (ImFont* font) { mMonoFont = font; }

  // callbacks
  void windowResize (int width, int height);
  virtual void drop (const std::vector<std::string>& dropItems) = 0;

  void mainUILoop();

private:
  cPlatform* mPlatform;
  cGraphics* mGraphics;

  ImFont* mMainFont;
  ImFont* mMonoFont;
  };

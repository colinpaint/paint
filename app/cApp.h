// cApp.h - base class - application context
#pragma once
//{{{  includes
#include <string>
#include <chrono>
#include <functional>

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
  void setResizeCallback (std::function<void (int width, int height)> callback) { mResizeCallback = callback; }
  void setDropCallback (std::function<void (std::vector<std::string> dropItems)> callback) { mDropCallback = callback; }

  void mainUILoop();
  void windowResize (int width, int height);

  // vars
  std::function <void (int width, int height)> mResizeCallback;
  std::function <void (std::vector<std::string> dropItems)> mDropCallback;

private:
  cPlatform* mPlatform;
  cGraphics* mGraphics;

  ImFont* mMainFont;
  ImFont* mMonoFont;
  };

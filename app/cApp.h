// cApp.h - base class - application context
#pragma once
//{{{  includes
#include <string>
#include <chrono>

class cPlatform;
class cGraphics;
struct ImFont;
//}}}

class cApp {
public:
  cApp (cPlatform& platform, cGraphics& graphics) : mPlatform(platform), mGraphics(graphics) {}
  virtual ~cApp() = default;

  // get interfaces
  cPlatform& getPlatform() const { return mPlatform; }
  cGraphics& getGraphics() const { return mGraphics; }

  // fonts
  ImFont* getMainFont() const { return mMainFont; }
  ImFont* getMonoFont() const { return mMonoFont; }

  void setMainFont (ImFont* font) { mMainFont = font; }
  void setMonoFont (ImFont* font) { mMonoFont = font; }

  std::chrono::system_clock::time_point getNow();

private:
  cPlatform& mPlatform;
  cGraphics& mGraphics;

  ImFont* mMainFont;
  ImFont* mMonoFont;
  };

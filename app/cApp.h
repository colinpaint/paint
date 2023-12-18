// cApp.h - base class - application context, platform,graphics interface, platform,graphics independent stuff
// - should work up as proper singleton
#pragma once
//{{{  includes
#include <string>
#include <chrono>
#include <vector>

// utils
#include "../common/basicTypes.h"
#include "../common/cLog.h"

class cPlatform;
class cGraphics;
struct ImFont;
//}}}

class cApp;
class iUI {
public:
  virtual ~iUI() = default;
  virtual void draw (cApp& app) = 0;
  };

class cApp {
public:
  //{{{
  class cOptions {
  public:
    cOptions() = default;
    virtual ~cOptions() = default;

    // vars
    eLogLevel mLogLevel = LOGINFO;

    cPoint mWindowSize = { 1920/2, 1080/2 };
    bool mFullScreen = false;
    bool mHasGui = true;
    bool mVsync = true;
    };
  //}}}

  cApp (const std::string& name, cOptions* options, iUI* ui);
  virtual ~cApp();

  // get interfaces
  std::string getName() const;
  cPlatform& getPlatform() const { return *mPlatform; }
  cGraphics& getGraphics() const { return *mGraphics; }

  // time of day
  std::chrono::system_clock::time_point getNow();

  // fonts
  ImFont* getMainFont() const { return mMainFont; }
  ImFont* getMonoFont() const { return mMonoFont; }
  void setMainFont (ImFont* font) { mMainFont = font; }
  void setMonoFont (ImFont* font) { mMonoFont = font; }

  // callbacks
  void windowResize (int width, int height);
  virtual void drop (const std::vector<std::string>& dropItems) = 0;

  void mainUILoop();

private:
  std::string mName;
  cOptions* mOptions;
  iUI* mUI;

  cPlatform* mPlatform = nullptr;
  cGraphics* mGraphics = nullptr;

  ImFont* mMainFont;
  ImFont* mMonoFont;
  };

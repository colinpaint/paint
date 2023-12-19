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

class cApp {
public:
  //{{{
  class cAppOptions : public cOptions {
  public:
    cAppOptions() = default;
    virtual ~cAppOptions() = default;

    //{{{
    bool parse (std::string param) {
    // parse params, true if recognised

      if (param == "log1")
        mLogLevel = LOGINFO1;
      else if (param == "log2")
        mLogLevel = LOGINFO2;
      else if (param == "log3")
        mLogLevel = LOGINFO3;
      else if (param == "full")
        mFullScreen;
      else if (param == "free")
        mVsync = false;
      else
        return false;

      return true;
      }
    //}}}
    //{{{
    void parse (int numArgs, char* args[]) {
    //  parse command line args to params

      for (int i = 1; i < numArgs; i++)
        parse (args[i]);
      }
    //}}}

    eLogLevel mLogLevel = LOGINFO;

    cPoint mWindowSize = { 1920/2, 1080/2 };
    bool mFullScreen = false;
    bool mVsync = true;

    bool mHasGui = true;
    };
  //}}}
  //{{{
  class iUI {
  public:
    virtual ~iUI() = default;

    virtual void draw (cApp& app) = 0;
    };
  //}}}

  cApp (const std::string& name, cAppOptions* options, iUI* ui);
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
  cAppOptions* mOptions;
  iUI* mUI;

  cPlatform* mPlatform = nullptr;
  cGraphics* mGraphics = nullptr;

  ImFont* mMainFont;
  ImFont* mMonoFont;
  };

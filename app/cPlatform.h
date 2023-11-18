// cPlatform.h - static register and base class
#pragma once
//{{{  includes
#include <string>

#include "../common/basicTypes.h"
//}}}

// abstract base class
class cPlatform {
public:
  cPlatform (const std::string& name, bool hasFullScreen, bool hasVsync)
    : mName(name), mHasFullScreen(hasFullScreen), mHasVsync(hasVsync) {}
  virtual ~cPlatform() = default;

  virtual bool init (const cPoint& windowSize) = 0;

  // gets
  std::string getName() const { return mName; }
  bool hasFullScreen() const { return mHasFullScreen; }
  bool getFullScreen() const { return mFullScreen; }

  bool hasVsync() { return mHasVsync; }
  bool getVsync() { return mVsync; }

  virtual cPoint getWindowSize() = 0;
  std::string getShaderVersion() { return mShaderVersion; }

  // sets
  virtual void setFullScreen (bool fullScreen) = 0;
  virtual void toggleFullScreen() = 0;

  virtual void setVsync (bool vsync) = 0;
  virtual void toggleVsync() = 0;

  // actions, should they be public?
  virtual bool pollEvents() = 0;
  virtual void newFrame() = 0;
  virtual void present() = 0;

protected:
  void setShaderVersion (const std::string& version) { mShaderVersion = version; }

  const std::string mName;
  const bool mHasFullScreen;
  const bool mHasVsync;

  bool mFullScreen = false;
  bool mVsync = true;

private:
  std::string mShaderVersion;
  };

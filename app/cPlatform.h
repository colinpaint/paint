// cPlatform.h - static register and base class
#pragma once
//{{{  includes
#include <string>

#include "../utils/cPointRectColor.h"
//}}}

class cPlatform {
public:
  cPlatform (bool hasFullScreen, bool hasVsync) : mHasFullScreen(hasFullScreen), mHasVsync(hasVsync) {}
  virtual ~cPlatform() = default;

  virtual bool init (const cPoint& windowSize) = 0;

  // gets
  bool hasFullScreen() { return mHasFullScreen; }
  bool getFullScreen() { return mFullScreen; }

  bool hasVsync() { return mHasVsync; }
  bool getVsync() { return mVsync; }

  virtual cPoint getWindowSize() = 0;

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
  const bool mHasFullScreen;
  const bool mHasVsync;

  bool mFullScreen = false;
  bool mVsync = true;
  };

// cPlatform.h - static register and base class
#pragma once
//{{{  includes
#include <string>

#include "../utils/cPointRectColor.h"
//}}}

class cPlatform {
public:
  virtual ~cPlatform() = default;

  virtual bool init (const cPoint& windowSize) = 0;

  // gets
  virtual cPoint getWindowSize() = 0;

  virtual bool hasVsync() = 0;
  virtual bool getVsync() = 0;

  virtual bool hasFullScreen() = 0;
  virtual bool getFullScreen() = 0;

  // sets
  virtual void setVsync (bool vsync) = 0;
  virtual void toggleVsync() = 0;

  virtual void setFullScreen (bool fullScreen) = 0;
  virtual void toggleFullScreen() = 0;

  // actions, should they be public?
  virtual bool pollEvents() = 0;
  virtual void newFrame() = 0;
  virtual void present() = 0;
  };

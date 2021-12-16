// cPlatform.h - static register and base class
#pragma once
//{{{  includes
#include <string>
#include <functional>

#include "../utils/cPointRectColor.h"
//}}}

class cPlatform {
public:
  static cPlatform& create (const cPoint& windowSize);

  // base class
  virtual bool init (const cPoint& windowSize) = 0;
  virtual void shutdown() = 0;

  // gets
  virtual cPoint getWindowSize() = 0;

  // - anonymous dx11 platform -> graphics interface
  virtual void* getDevice() = 0;
  virtual void* getDeviceContext() = 0;
  virtual void* getSwapChain() = 0;

  virtual bool hasVsync() = 0;
  virtual bool getVsync() = 0;

  virtual bool hasFullScreen() = 0;
  virtual bool getFullScreen() = 0;

  // sets
  virtual void setVsync (bool vsync) = 0;
  virtual void toggleVsync() = 0;

  virtual void setFullScreen (bool fullScreen) = 0;
  virtual void toggleFullScreen() = 0;

  // actions
  virtual bool pollEvents() = 0;
  virtual void newFrame() = 0;
  virtual void present() = 0;
  virtual void close() = 0;

  // callback
  void setResizeCallback (std::function<void (int width, int height)> callback) { mResizeCallback = callback; }
  void setDropCallback (std::function<void (std::vector<std::string> dropItems)> callback) { mDropCallback = callback; }

  // vars
  std::function <void (int programPid, int programSid)> mResizeCallback;
  std::function <void (std::vector<std::string> dropItems)> mDropCallback;
  };

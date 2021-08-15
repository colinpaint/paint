// cPlatform.h - platform abstract interface
#pragma once
#include "../graphics/cPointRect.h"
class cGraphics;

class cPlatform {
public:
  // static factory create
  static cPlatform& create();

  // abstract interface
  virtual bool init (const cPoint& windowSize, bool showViewports) = 0;
  virtual void shutdown();

  // gets
  virtual void* getDevice() = 0;
  virtual void* getDeviceContext() = 0;
  virtual void* getSwapChain() = 0;
  virtual cPoint getWindowSize() = 0;

  // sets
  using sizeCallbackFunc = void(*)(cGraphics* graphics, int width, int height);
  virtual void setSizeCallback (cGraphics* graphics, const sizeCallbackFunc sizeCallback) = 0;

  // actions
  virtual bool pollEvents() = 0;
  virtual void newFrame() = 0;
  virtual void present() = 0;
  };

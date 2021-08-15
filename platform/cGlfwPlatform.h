// cWin32Platform.h - platform abstract interface
#pragma once
#include "cPlatform.h"

class cGlfwPlatform : public cPlatform {
public:
  bool init (const cPoint& windowSize, bool showViewports) final;
  void shutdown() final;

  // gets
  void* getDevice() final;
  void* getDeviceContext() final;
  void* getSwapChain() final;
  cPoint getWindowSize() final;

  virtual void setSizeCallback (cGraphics* graphics, const sizeCallbackFunc sizeCallback) final;

  // actions
  bool pollEvents() final;
  void newFrame() final;
  void present() final;
  };

// cPlatform.h - platform singleton
#pragma once
//{{{  includes
// glm
#include <vec2.hpp>

#include "../graphics/cPointRect.h"

class cGraphics;
//}}}

class cPlatform {
public:
  using sizeCallbackFunc = void(*)(cGraphics* graphics, int width, int height);
  //{{{
  static cPlatform& getInstance() {
  // singleton pattern create
  // - thread safe
  // - allocate with `new` in case singleton is not trivially destructible.

    static cPlatform* platform = new cPlatform();
    return *platform;
    }
  //}}}

  bool init (const cPoint& windowSize, bool showViewports);
  void shutdown();

  // gets
  void* getDevice();
  void* getDeviceContext();
  void* getSwapChain();
  cPoint getWindowSize();

  void setSizeCallback (cGraphics* graphics, const sizeCallbackFunc sizeCallback);

  // actions
  bool pollEvents();
  void newFrame();
  void present();

private:
  // singleton pattern fluff
  cPlatform() = default;

  // delete copy/move so extra instances can't be created/moved
  cPlatform (const cPlatform&) = delete;
  cPlatform& operator = (const cPlatform&) = delete;
  cPlatform (cPlatform&&) = delete;
  cPlatform& operator = (cPlatform&&) = delete;
  };

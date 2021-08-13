// cPlatform.h - platform singleton
#pragma once
//{{{  includes
// glm
#include <vec2.hpp>

#include "../graphics/cPointRect.h"

class cGraphics;
class cCanvas;
//}}}

struct ID3D11Device;
struct ID3D11DeviceContext;

class cPlatform {
public:
  using sizeCallbackFunc = void(*)();
  //{{{
  static cPlatform& getInstance() {
  // singleton pattern create
  // - thread safe
  // - allocate with `new` in case singleton is not trivially destructible.

    static cPlatform* platform = new cPlatform();
    return *platform;
    }
  //}}}

  bool init (const cPoint& windowSize, bool showViewports, const sizeCallbackFunc sizeCallback);
  void shutdown();

  // gets
  cPoint getWindowSize();
  ID3D11Device* getDevice();
  ID3D11DeviceContext* getDeviceContext();

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

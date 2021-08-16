// cPlatform.h - platform abstract interface
#pragma once
//{{{  includes
#include <string>
#include <map>
#include <functional>

#include "../graphics/cPointRect.h"
class cGraphics;
//}}}

class cPlatform {
public:
  // statics
  static cPlatform& createByName (const std::string& name);
  static void listClasses();

  // callback
  //{{{
  void setResizeCallback (std::function<void (int width, int height)> callback) {
    mResizeCallback = callback;
    }
  //}}}

  // abstract interface
  virtual bool init (const cPoint& windowSize, bool showViewports) = 0;
  virtual void shutdown() = 0;

  // gets
  virtual void* getDevice() = 0;
  virtual void* getDeviceContext() = 0;
  virtual void* getSwapChain() = 0;
  virtual cPoint getWindowSize() = 0;

  // actions
  virtual bool pollEvents() = 0;
  virtual void newFrame() = 0;
  virtual void present() = 0;

  std::function <void (int programPid, int programSid)> mResizeCallback;

protected:
  using createFunc = cPlatform*(*)(const std::string& name);
  static bool registerClass (const std::string& name, const createFunc factoryMethod);
  static std::map<const std::string, createFunc>& getClassRegister();
  };

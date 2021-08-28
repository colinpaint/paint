// cPlatform.h - static register and base class
#pragma once
//{{{  includes
#include <string>
#include <chrono>
#include <map>
#include <functional>

#include "../utils/cPointRectColor.h"
//}}}

class cPlatform {
public:
  // static register
  static cPlatform& createByName (const std::string& name, const cPoint& windowSize,
                                  bool showViewports, bool vsync);
  static void listRegisteredClasses();

  // base class
  virtual void shutdown() = 0;

  // gets
  virtual cPoint getWindowSize() = 0;
  std::chrono::system_clock::time_point now();

  // - anonymous dx11 platform -> graphics interface
  virtual void* getDevice() = 0;
  virtual void* getDeviceContext() = 0;
  virtual void* getSwapChain() = 0;

  // actions
  virtual bool pollEvents() = 0;
  virtual void newFrame() = 0;
  virtual void present() = 0;

  // callback
  void setResizeCallback (std::function<void (int width, int height)> callback) { mResizeCallback = callback; }
  void setDropCallback (std::function<void (std::vector<std::string> dropItems)> callback) { mDropCallback = callback; }

  std::function <void (int programPid, int programSid)> mResizeCallback;
  std::function <void (std::vector<std::string> dropItems)> mDropCallback;

protected:
  // static register
  using createFunc = cPlatform*(*)(const std::string& name);
  static bool registerClass (const std::string& name, const createFunc factoryMethod);
  static std::map<const std::string, createFunc>& getClassRegister();

  virtual bool init (const cPoint& windowSize, bool showViewports, bool vsync) = 0;
  };

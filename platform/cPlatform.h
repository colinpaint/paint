// cPlatform.h - platform abstract interface
#pragma once
//{{{  includes
#include <string>
#include <map>

#include "../graphics/cPointRect.h"
class cGraphics;
//}}}

class cPlatform {
public:
  using createFunc = cPlatform*(*)(const std::string& name);

  //{{{
  static cPlatform& cPlatform::createByName (const std::string& name) {
    return *getClassRegister()[name](name);
    }
  //}}}
  static void listClasses();

  // abstract interface
  virtual bool init (const cPoint& windowSize, bool showViewports) = 0;
  virtual void shutdown() = 0;

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

protected:
  //{{{
  static bool registerClass (const std::string& name, const createFunc factoryMethod) {
  // trickery - function needs to be called by a derived class inside a static context

    if (getClassRegister().find (name) == getClassRegister().end()) {
      // className not found - add to classRegister map
      getClassRegister().insert (std::make_pair (name, factoryMethod));
      return true;
      }
    else
      return false;
    }
  //}}}

private:
  //{{{
  static std::map<const std::string, createFunc>& getClassRegister() {
  // trickery - static map inside static method ensures map is created before any use
    static std::map<const std::string, createFunc> mClassRegistry;
    return mClassRegistry;
    }
  //}}}
  };

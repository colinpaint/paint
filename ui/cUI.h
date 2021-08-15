// cUI.h - UI static manager and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <map>

#include "../graphics/cPointRect.h"
class cGraphics;
class cCanvas;
//}}}

class cUI {
private:
  using createFuncType = cUI*(*)(const std::string& name);

public:
  // static manager, static init register
  //{{{
  static bool registerClass (const std::string& name, const createFuncType createFunc) {
  // register class createFunc by name to classRegister, add instance to instances

    if (getClassRegister().find (name) == getClassRegister().end()) {
      // class name not found - add to classRegister map
      getClassRegister().insert (std::make_pair (name, createFunc));

      // create instance of class and add to instances map
      getInstances().insert (std::make_pair (name, createFunc (name)));
      return true;
      }
    else
      return false;
    }
  //}}}
  //{{{
  static cUI* createByName (const std::string& name) {
  // create class by name from classRegister, add instance to instances

    auto uiIt = getInstances().find (name);
    if (uiIt == getInstances().end()) {
      auto ui = getClassRegister()[name](name);
      getInstances().insert (std::make_pair (name, ui));
      return ui;
      }
    else
      return uiIt->second;
    }
  //}}}
  static void draw (cCanvas& canvas, cGraphics& graphics);

  // base class
  cUI (const std::string& name) : mName(name) {}
  virtual ~cUI() = default;

  std::string getName() const { return mName; }

  virtual void addToDrawList (cCanvas& canvas, cGraphics& graphics) = 0;

private:
  //{{{
  static std::map<const std::string, createFuncType>& getClassRegister() {
  // static map inside static method ensures map is created before use
    static std::map<const std::string, createFuncType> mClassRegister;
    return mClassRegister;
    }
  //}}}
  //{{{
  static std::map<const std::string, cUI*>& getInstances() {
  // static map inside static method ensures map is created before use
    static std::map<const std::string, cUI*> mInstances;
    return mInstances;
    }
  //}}}

  // registered name
  std::string mName;
  };

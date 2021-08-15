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

// cUI
class cUI {
private:
  using createFuncType = cUI*(*)(const std::string& name);

public:
  // static manager, static init registration, main UI draw
  static bool registerClass (const std::string& name, const createFuncType createFunc);
  static cUI* createByName (const std::string& name);
  static void draw (cCanvas& canvas, cGraphics& graphics);

  //
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

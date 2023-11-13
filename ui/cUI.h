// cUI.h - UI static manager and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>

#include "../app/cApp.h"

#include "../common/date.h"
#include "../common/cPointRectColor.h"

class cGraphics;
class cPlatform;
class cCanvas;
//}}}

class cUI {
public:
  // static register
  static cUI* createByName (const std::string& name);
  static void listRegisteredClasses();
  static void listInstances();

  // static render
  static void render (cApp& app);

  // base class
  cUI (const std::string& name) : mName(name) {}
  virtual ~cUI() = default;

  std::string getName() const { return mName; }

  virtual void addToDrawList (cApp& app) = 0;

protected:
  using createFuncType = cUI*(*)(const std::string& name);
  static bool registerClass (const std::string& name, const createFuncType createFunc);

private:
  // static register
  static std::map<const std::string, createFuncType>& getClassRegister();
  static std::map<const std::string, cUI*>& getInstanceRegister();

  // base class registered name
  const std::string mName;
  };

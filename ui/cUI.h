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
public:
  // static register
  static cUI* createByName (const std::string& name);
  static void listClasses();
  static void listInstances();

  // static draw
  static void draw (cCanvas& canvas, cGraphics& graphics);

  // base class
  cUI (const std::string& name) : mName(name) {}
  virtual ~cUI() = default;

  std::string getName() const { return mName; }

  virtual void addToDrawList (cCanvas& canvas, cGraphics& graphics) = 0;

protected:
  using createFuncType = cUI*(*)(const std::string& name);
  static bool registerClass (const std::string& name, const createFuncType createFunc);

private:
  // static register
  static std::map<const std::string, createFuncType>& getClassRegister();
  static std::map<const std::string, cUI*>& getInstanceRegister();

  // base class registered name
  std::string mName;
  };

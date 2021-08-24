// cUI.h - UI static manager and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include "../utils/date.h"

struct ImVec2;
#include "../utils/cPointRectColor.h"
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

  // static draw
  static void draw (cCanvas& canvas, cGraphics& graphics, cPlatform& platform, cPoint windowSize);

  // base class
  cUI (const std::string& name) : mName(name) {}
  virtual ~cUI() = default;

  std::string getName() const { return mName; }

  virtual void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform) = 0;

protected:
  bool clockButton (const std::string& label, std::chrono::system_clock::time_point timePoint, const ImVec2& size_arg);
  bool toggleButton (const std::string& label, bool toggleOn, const ImVec2& size_arg);
  unsigned interlockedButtons (const std::vector<std::string>& buttonVector, unsigned index, const ImVec2& size_arg);

  using createFuncType = cUI*(*)(const std::string& name);
  static bool registerClass (const std::string& name, const createFuncType createFunc);

private:
  // static register
  static std::map<const std::string, createFuncType>& getClassRegister();
  static std::map<const std::string, cUI*>& getInstanceRegister();

  // base class registered name
  const std::string mName;
  };

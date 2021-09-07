// cUI.h - UI static manager and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <chrono>

#include "cApp.h"

#include "../utils/date.h"

// imgui
#include "../imgui/imgui.h"

// utils
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
  static void draw (cApp* app);

  // base class
  cUI (const std::string& name) : mName(name) {}
  virtual ~cUI() = default;

  std::string getName() const { return mName; }

  virtual void addToDrawList (cApp* app) = 0;

protected:
  bool clockButton (const std::string& label, std::chrono::system_clock::time_point timePoint, const ImVec2& size_arg);
  bool toggleButton (const std::string& label, bool toggleOn, const ImVec2& size_arg = ImVec2(0, 0));
  unsigned interlockedButtons (const std::vector<std::string>& buttonVector, unsigned index, const ImVec2& size_arg);

  static void printHex (uint8_t* ptr, uint32_t numBytes, uint32_t columnsPerRow, uint32_t address, bool full);

  using createFuncType = cUI*(*)(const std::string& name);
  static bool registerClass (const std::string& name, const createFuncType createFunc);

private:
  // static register
  static std::map<const std::string, createFuncType>& getClassRegister();
  static std::map<const std::string, cUI*>& getInstanceRegister();

  // base class registered name
  const std::string mName;
  };

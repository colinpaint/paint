// cUIMan.h - static manager registering cUI classes
#pragma once
//{{{  includes
// includes
#include <cstdint>
#include <string>
#include <map>

#include "../graphics/cPointRect.h"
class cCanvas;
class cUI;
class cGraphics;
//}}}

// cUIMan
class cUIMan {
// - registerClass
//   - create and add an instance of registered class to instance map
//     - could add delete and create by name
//       - if not classRegister is unused, instances map could be a vector
private:
  using createFuncType = cUI*(*)(const std::string& name);

public:
  static bool registerClass (const std::string& name, const createFuncType createFunc);
  static cUI* createByName (const std::string& name);

  static void draw (cCanvas& canvas, cGraphics& graphics);

private:
  static std::map<const std::string, createFuncType>& getClassRegister() {
  // static map inside static method ensures map is created before use
    static std::map<const std::string, createFuncType> mClassRegister;
    return mClassRegister;
    }

  static std::map<const std::string, cUI*>& getInstances() {
  // static map inside static method ensures map is created before use
    static std::map<const std::string, cUI*> mInstances;
    return mInstances;
    }
  };

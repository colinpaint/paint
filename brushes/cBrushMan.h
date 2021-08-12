// cBrushMan.h - brush manager static class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <map>
class cBrush;
//}}}

// cBrushMan
class cBrushMan {
private:
  using createFunc = cBrush*(*)(const std::string& name, float radius);

public:
  static cBrush* createByName (const std::string& name, float radius);

  static bool registerClass (const std::string& name, const createFunc factoryMethod);

  static std::map<const std::string, createFunc>& getClassRegister() {
  // trickery - static map inside static method ensures map is created before any use
    static std::map<const std::string, createFunc> mClassRegistry;
    return mClassRegistry;
    }

  // gets
  static cBrush* getCurBrush() { return mCurBrush; }
  static bool isCurBrushByName (const std::string& name);

  // sets
  static cBrush* setCurBrushByName (const std::string& name, float radius);

private:
  inline static cBrush* mCurBrush = nullptr;
  };

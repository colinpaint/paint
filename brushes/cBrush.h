// cBrush.h - static manager and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <map>

// glm
#include <vec2.hpp>
#include <vec4.hpp>

#include "../graphics/cPointRect.h"
class cGraphics;
class cFrameBuffer;
//}}}

class cBrush {
private:
  using createFunc = cBrush*(*)(const std::string& name, float radius, cGraphics& graphics);

public:
  // static manager, inline registration of brushes, select curBrush by name
  //{{{
  static cBrush* createByName (const std::string& name, float radius, cGraphics& graphics) {
    return getClassRegister()[name](name, radius, graphics);
    }
  //}}}
  //{{{
  static std::map<const std::string, createFunc>& getClassRegister() {
  // trickery - static map inside static method ensures map is created before any use
    static std::map<const std::string, createFunc> mClassRegistry;
    return mClassRegistry;
    }
  //}}}

  // static curBrush
  static cBrush* getCurBrush() { return mCurBrush; }
  //{{{
  static bool isCurBrushByName (const std::string& name) {
    return mCurBrush ? name == mCurBrush->getName() : false;
    }
  //}}}
  static cBrush* setCurBrushByName (const std::string& name, float radius, cGraphics& graphics);

  // base class
  cBrush (const std::string& name, float radius) : mName(name) {}
  virtual ~cBrush() = default;

  std::string getName() const { return mName; }
  //{{{
  glm::vec4 getColor() {
    return glm::vec4(static_cast<float>(mR) / 255.f,
                     static_cast<float>(mG) / 255.f,
                     static_cast<float>(mB) / 255.f,
                     static_cast<float>(mA) / 255.f);
    }
  //}}}

  float getRadius() { return mRadius; }
  float getBoundRadius() { return mRadius + 1.f; }
  cRect getBoundRect (glm::vec2 pos, cFrameBuffer* frameBuffer);

  void setColor (glm::vec4 color);
  void setColor (float r, float g, float b, float a);
  void setColor (uint8_t r, uint8_t g, uint8_t b, uint8_t a);

  // virtuals
  virtual void setRadius (float radius) { mRadius = radius; }
  virtual void paint (glm::vec2 pos, bool first, cFrameBuffer* frameBuffer, cFrameBuffer* frameBuffer1) = 0;

protected:
  float mRadius = 0.f;

  uint8_t mR = 0;
  uint8_t mG = 0;
  uint8_t mB = 0;
  uint8_t mA = 255;

  glm::vec2 mPrevPos = glm::vec2(0.f,0.f);

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
  const std::string mName;

  // static curBrush
  inline static cBrush* mCurBrush = nullptr;
  };

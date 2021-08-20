// cBrush.h - brush static manager and base class
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <map>

#include "../utils/cPointRectColor.h"
#include "../utils/cVecMat.h"

class cGraphics;
class cFrameBuffer;
//}}}

class cBrush {
private:
  using createFunc = cBrush*(*)(cGraphics& graphics, const std::string& name, float radius);

public:
  // static register
  static cBrush* createByName (cGraphics& graphics, const std::string& name, float radius);
  static std::map<const std::string, createFunc>& getClassRegister();
  static void listRegisteredClasses();

  // static curBrush
  static cBrush* getCurBrush() { return mCurBrush; }
  static bool isCurBrushByName (const std::string& name);
  static cBrush* setCurBrushByName (cGraphics& graphics, const std::string& name, float radius);

  // base class
  cBrush (const std::string& name, float radius) : mName(name) {}
  virtual ~cBrush() = default;

  std::string getName() const { return mName; }
  //{{{
  cColor getColor() {
    return cColor(static_cast<float>(mR) / 255.f,
                  static_cast<float>(mG) / 255.f,
                  static_cast<float>(mB) / 255.f,
                  static_cast<float>(mA) / 255.f);
    }
  //}}}

  float getRadius() { return mRadius; }
  float getBoundRadius() { return mRadius + 1.f; }
  cRect getBoundRect (cVec2 pos, const cFrameBuffer& frameBuffer);

  void setColor (const cColor& color);
  void setColor (float r, float g, float b, float a);
  void setColor (uint8_t r, uint8_t g, uint8_t b, uint8_t a);

  // virtuals
  virtual void setRadius (float radius) { mRadius = radius; }
  virtual void paint (cVec2 pos, bool first, cFrameBuffer& frameBuffer, cFrameBuffer& frameBuffer1) = 0;

protected:
  float mRadius = 0.f;

  uint8_t mR = 0;
  uint8_t mG = 0;
  uint8_t mB = 0;
  uint8_t mA = 255;

  cVec2 mPrevPos = cVec2(0.f,0.f);

protected:
  // static register
  static bool registerClass (const std::string& name, const createFunc factoryMethod);

private:
  // static curBrush
  inline static cBrush* mCurBrush = nullptr;

  // base class registered name
  const std::string mName;
  };

// cBrush.cpp - paint brush to frameBuffer
//{{{  includes
#include "cBrush.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include "../app/cGraphics.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

// static curBrush
//{{{
cBrush* cBrush::setCurBrushByName (cGraphics& graphics, const string& name, float radius) {

  if (!isCurBrushByName (name)) {
    // default color if no curBrush
    cColor color (0.7f,0.7f,0.f, 1.f);

    if (mCurBrush)
      color = mCurBrush->getColor();
    delete mCurBrush;

    mCurBrush = createByName (graphics, name, radius);
    mCurBrush->setColor (color);
    cLog::log (LOGINFO, "setCurBrushByName " + name);
    }

  return mCurBrush;
  }
//}}}
//{{{
map<const string, cBrush::createFunc>& cBrush::getClassRegister() {
// static map inside static method ensures map is created before any use

  static map<const string, createFunc> mClassRegistry;
  return mClassRegistry;
  }
//}}}
//{{{
void cBrush::listRegisteredClasses() {

  cLog::log (LOGINFO, "brush register");
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, fmt::format ("- {}", ui.first));
  }
//}}}

//{{{
cBrush* cBrush::createByName (cGraphics& graphics, const string& name, float radius) {
  return getClassRegister()[name](graphics, name, radius);
  }
//}}}
//{{{
bool cBrush::isCurBrushByName (const string& name) {
  return mCurBrush ? name == mCurBrush->getName() : false;
  }
//}}}

// gets
//{{{
cRect cBrush::getBoundRect (cVec2 pos, const cFrameBuffer& frameBuffer) {
// return boundRect of line from mPrevPos to pos of brush mRadius, cliiped to frameBuffer

  const float boundRadius = getBoundRadius();

  const float widthF = static_cast<float>(frameBuffer.getSize().x);
  const float heightF = static_cast<float>(frameBuffer.getSize().y);

  const float minx = max (0.f, min (pos.x - boundRadius, mPrevPos.x - boundRadius));
  const float maxx = max (0.f, min (max (pos.x + boundRadius, mPrevPos.x + boundRadius), widthF));

  const float miny = max (0.f, min (pos.y - boundRadius, mPrevPos.y - boundRadius));
  const float maxy = max (0.f, min (max (pos.y + boundRadius, mPrevPos.y + boundRadius), heightF));

  return cRect (static_cast<int>(minx), static_cast<int>(miny),
                static_cast<int>(maxx), static_cast<int>(maxy));
  }
//}}}

// sets
//{{{
void cBrush::setColor (const cColor& color) {

  mR = static_cast<uint8_t>(color.r * 255.f);
  mG = static_cast<uint8_t>(color.g * 255.f);
  mB = static_cast<uint8_t>(color.b * 255.f);
  mA = static_cast<uint8_t>(color.a * 255.f);
  }
//}}}
//{{{
void cBrush::setColor (float r, float g, float b, float a) {

  mR = static_cast<uint8_t>(r * 255.f);
  mG = static_cast<uint8_t>(g * 255.f);
  mB = static_cast<uint8_t>(b * 255.f);
  mA = static_cast<uint8_t>(a * 255.f);
  }
//}}}
//{{{
void cBrush::setColor (uint8_t r, uint8_t g, uint8_t b, uint8_t a) {

  mR = r;
  mG = g;
  mB = b;
  mA = a;
  }
//}}}

// protected:
//{{{
bool cBrush::registerClass (const string& name, const createFunc factoryMethod) {

  if (getClassRegister().find (name) == getClassRegister().end()) {
    // className not found - add to classRegister map
    getClassRegister().insert (make_pair (name, factoryMethod));
    return true;
    }
  else
    return false;
  }
//}}}

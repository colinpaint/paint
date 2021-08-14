// cBrush.cpp - paint brush to frameBuffer
//{{{  includes
#include "cBrush.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// glm
#include <vec2.hpp>
#include <vec4.hpp>

#include "../graphics/cGraphics.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

// cBrush
cBrush::cBrush (const string& name, float radius) : mName(name){}

// - gets
//{{{
glm::vec4 cBrush::getColor() {

  return glm::vec4(static_cast<float>(mR) / 255.f,
                   static_cast<float>(mG) / 255.f,
                   static_cast<float>(mB) / 255.f,
                   static_cast<float>(mA) / 255.f);
  }
//}}}
//{{{
cRect cBrush::getBoundRect (glm::vec2 pos, cFrameBuffer* frameBuffer) {
// return boundRect of line from mPrevPos to pos of brush mRadius, cliiped to frameBuffer

  const float boundRadius = getBoundRadius();

  const float widthF = static_cast<float>(frameBuffer->getSize().x);
  const float heightF = static_cast<float>(frameBuffer->getSize().y);

  const float minx = max (0.f, min (pos.x - boundRadius, mPrevPos.x - boundRadius));
  const float maxx = max (0.f, min (max (pos.x + boundRadius, mPrevPos.x + boundRadius), widthF));

  const float miny = max (0.f, min (pos.y - boundRadius, mPrevPos.y - boundRadius));
  const float maxy = max (0.f, min (max (pos.y + boundRadius, mPrevPos.y + boundRadius), heightF));

  return cRect (static_cast<int>(minx), static_cast<int>(miny),
                static_cast<int>(maxx), static_cast<int>(maxy));
  }
//}}}

// - sets
//{{{
void cBrush::setColor (glm::vec4 color) {

  mR = static_cast<uint8_t>(color.x * 255.f);
  mG = static_cast<uint8_t>(color.y * 255.f);
  mB = static_cast<uint8_t>(color.z * 255.f);
  mA = static_cast<uint8_t>(color.w * 255.f);
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

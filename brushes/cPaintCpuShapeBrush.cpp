// cPaintCpuShapeBrush.cpp
//{{{  includes
#include "cPaintCpuShapeBrush.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// glm
#include <vec2.hpp>
#include <vec4.hpp>

#include "../graphics/cFrameBuffer.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

// cPaintCpuShapeBrush
//{{{
cPaintCpuShapeBrush::cPaintCpuShapeBrush (const string& className, float radius)
    : cPaintCpuBrush(className, radius) {

  setRadius (radius);
  }
//}}}
//{{{
cPaintCpuShapeBrush::~cPaintCpuShapeBrush() {
  free (mShape);
  }
//}}}

//{{{
void cPaintCpuShapeBrush::setRadius (float radius) {

  cPaintCpuBrush::setRadius (radius);

  mSubPixels = 4;
  mSubPixelResolution = 1.f / mSubPixels;

  free (mShape);
  mShape = static_cast<uint8_t*>(malloc (mSubPixels * mSubPixels  * mShapeSize * mShapeSize));

  auto shape = mShape;
  for (int ySub = 0; ySub < mSubPixels; ySub++)
    for (int xSub = 0; xSub < mSubPixels; xSub++)
      for (int j = -mShapeRadius; j <= mShapeRadius; j++)
        for (int i = -mShapeRadius; i <= mShapeRadius; i++)
          *shape++ = getPaintShape (i - (xSub * mSubPixelResolution), j - (ySub * mSubPixelResolution), mRadius);
  }
//}}}

// - private
//{{{
void cPaintCpuShapeBrush::reportShape() {

  uint8_t* shapePtr = mShape;
  for (int y = 0; y < mShapeSize; y++) {
    string str;
    for (int x = 0; x < mShapeSize; x++)
      str += format ("{:3d} ", *shapePtr++);
    cLog::log (LOGINFO, str);
    }
  }
//}}}

//{{{
void cPaintCpuShapeBrush::stamp (glm::vec2 pos, cFrameBuffer* frameBuffer) {
// stamp brushShape into image, clipped by width,height to pos, update mPrevPos

  int32_t width = frameBuffer->getSize().x;
  int32_t height = frameBuffer->getSize().y;

  // x
  int32_t xInt = static_cast<int32_t>(pos.x);
  int32_t leftClipShape = -min(0, xInt - mShapeRadius);
  int32_t rightClipShape = max(0, xInt + mShapeRadius + 1 - width);
  int32_t xFirst = xInt - mShapeRadius + leftClipShape;
  float xSubPixelFrac = pos.x - xInt;

  // y
  int32_t yInt = static_cast<int32_t>(pos.y);
  int32_t topClipShape = -min(0, yInt - mShapeRadius);
  int32_t botClipShape = max(0, yInt + mShapeRadius + 1 - height);
  int32_t yFirst = yInt - mShapeRadius + topClipShape;
  float ySubPixelFrac = pos.y - yInt;

  // point to first image pix
  uint8_t* frame = frameBuffer->getPixels() + ((yFirst * width) + xFirst) * 4;
  int32_t frameRowInc = (width - mShapeSize + leftClipShape + rightClipShape) * 4;

  int32_t xSub = static_cast<int>(xSubPixelFrac / mSubPixelResolution);
  int32_t ySub = static_cast<int>(ySubPixelFrac / mSubPixelResolution);

  // point to first clipped shape pix
  uint8_t* shape = mShape;
  shape += ((ySub * mSubPixels) + xSub) * mShapeSize * mShapeSize;
  shape += (topClipShape * mShapeSize) + leftClipShape;

  int shapeRowInc = rightClipShape + leftClipShape;

  for (int32_t j = -mShapeRadius + topClipShape; j <= mShapeRadius - botClipShape; j++) {
    for (int32_t i = -mShapeRadius + leftClipShape; i <= mShapeRadius - rightClipShape; i++) {
      uint16_t foreground = *shape++;
      if (foreground > 0) {
        // stamp some foreground
        foreground = (foreground * mA) / 255;
        if (foreground >= 255) {
          // all foreground
          *frame++ = mR;
          *frame++ = mG;
          *frame++ = mB;
          *frame++ = mA;
          }
        else {
          // blend foreground into background
          uint16_t background = 255 - foreground;
          uint16_t r = (mR * foreground) + (*frame * background);
          *frame++ = r / 255;
          uint16_t g = (mG * foreground) + (*frame * background);
          *frame++ = g / 255;
          uint16_t b = (mB * foreground) + (*frame * background);
          *frame++ = b / 255;
          uint16_t a = (mA * foreground) + (*frame * background);
          *frame++ = a / 255;
          }
        }
      else
        frame += 4;
      }

    // onto next row
    frame += frameRowInc;
    shape += shapeRowInc;
    }

  mPrevPos = pos;
  }
//}}}

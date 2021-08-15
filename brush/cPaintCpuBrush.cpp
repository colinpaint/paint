// cPaintCpuBrush.cpp - paint brush to frameBuffer
//{{{  includes
#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// glm
#include <vec2.hpp>
#include <vec4.hpp>
#include <gtx/norm.hpp>

#include "cBrush.h"
#include "../graphics/cGraphics.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

// cPaintCpuBrush
class cPaintCpuBrush : public cBrush {
public:
  //{{{
  cPaintCpuBrush (const string& className, float radius, cGraphics& graphics)
      : cBrush(className, radius) {
    setRadius (radius);
    }
  //}}}
  virtual ~cPaintCpuBrush() = default;
  //{{{
  void setRadius (float radius) override {

    cBrush::setRadius (radius);
    mShapeRadius = static_cast<unsigned>(ceil(radius));
    mShapeSize = (2 * mShapeRadius) + 1;
    }
  //}}}
  //{{{
  void paint (glm::vec2 pos, bool first, cFrameBuffer* frameBuffer, cFrameBuffer* frameBuffer1) {

    if (first) {
      stamp (pos, frameBuffer);
      frameBuffer->pixelsChanged (getBoundRect(pos, frameBuffer));
      }

    else {
      cRect boundRect = getBoundRect (pos, frameBuffer);

      // draw stamps from mPrevPos to pos
      glm::vec2 diff = pos - mPrevPos;
      float length = sqrtf (glm::length2 (diff));
      float overlap = mRadius / 2.f;

      if (length >= overlap) {
        glm::vec2 inc = diff * (overlap / length);

        unsigned numStamps = static_cast<unsigned>(length / overlap);
        for (unsigned i = 0; i < numStamps; i++)
          stamp (mPrevPos + inc, frameBuffer);
        }

      frameBuffer->pixelsChanged (boundRect);
      }
    }
  //}}}

protected:
  //{{{
  uint8_t getPaintShape (float i, float j, float radius) {
    return static_cast<uint8_t>(255.f * (1.f - clamp (sqrtf((i*i) + (j*j)) - radius, 0.f, 1.f)));
    }
  //}}}

  int32_t mShapeRadius = 0;
  int32_t mShapeSize = 0;

private:
  //{{{
  static cBrush* createBrush (const std::string& className, float radius, cGraphics& graphics) {
    return new cPaintCpuBrush (className, radius, graphics);
    }
  //}}}
  //{{{
  virtual void stamp (glm::vec2 pos, cFrameBuffer* frameBuffer) {
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

    // iterate j then i, -radius to +radius clipped by frame, offset by x,y SubPixelFrac
    for (int32_t j = -mShapeRadius + topClipShape; j <= mShapeRadius - botClipShape; j++) {
      for (int32_t i = -mShapeRadius + leftClipShape; i <= mShapeRadius - rightClipShape; i++) {
        // calc shapePixel on the fly
        uint8_t shapePixel = getPaintShape (i - xSubPixelFrac, j - ySubPixelFrac, mRadius);
        if (shapePixel > 0) {
          // stamp some foreground colour
          uint16_t foreground = (shapePixel * mA) / 255;
          if (foreground >= 255) {
            // all foreground colour
            *frame++ = mR;
            *frame++ = mG;
            *frame++ = mB;
            *frame++ = mA;
            }
          else {
            // blend foreground colour into background frame
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
      }

    mPrevPos = pos;
    }
  //}}}

  // register paintCpu brush with the static manager
  inline static const bool mRegistered = registerClass ("paintCpu", &createBrush);
  };


// cPaintCpuShapeBrush
class cPaintCpuShapeBrush : public cPaintCpuBrush {
public:
  //{{{
  cPaintCpuShapeBrush (const string& className, float radius, cGraphics& graphics)
      : cPaintCpuBrush(className, radius, graphics) {

    setRadius (radius);
    }
  //}}}
  //{{{
  ~cPaintCpuShapeBrush() {
    free (mShape);
    }
  //}}}
  //{{{
  void setRadius (float radius) final {

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

private:
  //{{{
  static cBrush* createBrush (const std::string& className, float radius, cGraphics& graphics) {
    return new cPaintCpuShapeBrush (className, radius, graphics);
    }
  //}}}
  //{{{
  void reportShape() {

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
  void stamp (glm::vec2 pos, cFrameBuffer* frameBuffer) final {
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

  // vars
  int32_t mSubPixels = 4;
  float mSubPixelResolution = 0.f;

  uint8_t* mShape = nullptr;
  float mCreatedShapeRadius = 0.f;

  // register paintCpuShape brush with the static manager
  inline static const bool mRegistered = registerClass ("paintCpuShape", &createBrush);
  };

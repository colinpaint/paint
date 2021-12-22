// cPaintBrush.cpp - paint brush to Target - 3 versions
//{{{  includes
#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include "cBrush.h"
#include "../app/cGraphics.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

// cPaintGpuBrush - gpu shader line segment
class cPaintGpuBrush : public cBrush {
public:
  //{{{
  cPaintGpuBrush (cGraphics& graphics, const string& className, float radius)
      : cBrush(className, radius), mGraphics(graphics) {

    mShader = graphics.createPaintShader();
    setRadius (radius);
    }
  //}}}
  //{{{
  ~cPaintGpuBrush() {
    delete mShader;
    }
  //}}}
  //{{{
  void paint (cVec2 pos, bool first, cTarget& target, cTarget& target1) final {

    if (first)
      mPrevPos = pos;

    cRect boundRect = getBoundRect (pos, target);
    if (!boundRect.isEmpty()) {
      const float widthF = static_cast<float>(target.getSize().x);
      const float heightF = static_cast<float>(target.getSize().y);

      // target, boundRect probably ignored for Target1
      target1.setTarget (boundRect);
      target1.invalidate();
      target1.setBlend();

      // shader
      mShader->use();

      cMat4x4 orthoProjection (0.f, widthF, 0.f, heightF, -1.f, 1.f);
      mShader->setModelProjection (cMat4x4(), orthoProjection);

      mShader->setStroke (pos, mPrevPos, getRadius(), getColor());

      // source
      target.setSource();
      target.checkStatus();
      //target->reportInfo();

      // draw boundRect to Target1 target
      cQuad* quad = mGraphics.createQuad (target.getSize(), boundRect);
      quad->draw();
      delete quad;

      // blit boundRect target1 back to target
      target.blit (target1, boundRect.getTL(), boundRect);
      }

    mPrevPos = pos;
    }
  //}}}

private:
  cGraphics& mGraphics;
  cPaintShader* mShader = nullptr;

  //{{{
  static cBrush* create (cGraphics& graphics, const std::string& className, float radius) {
    return new cPaintGpuBrush (graphics, className, radius);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("paintGpu", &create);
  };


// cPaintCpuBrush - cpu stamp, calc shape pixels on the fly
class cPaintCpuBrush : public cBrush {
public:
  //{{{
  cPaintCpuBrush (cGraphics& graphics, const string& className, float radius)
      : cBrush(className, radius) {
    (void)graphics;
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
  void paint (cVec2 pos, bool first, cTarget& target, cTarget& target1) final {

    (void)target1;
    if (first) {
      stamp (pos, target);
      target.pixelsChanged (getBoundRect(pos, target));
      }

    else {
      cRect boundRect = getBoundRect (pos, target);

      // draw stamps from mPrevPos to pos
      cVec2 diff = pos - mPrevPos;
      float length = diff.magnitude();
      float overlap = mRadius / 2.f;

      if (length >= overlap) {
        cVec2 inc = diff * (overlap / length);

        unsigned numStamps = static_cast<unsigned>(length / overlap);
        for (unsigned i = 0; i < numStamps; i++)
          stamp (mPrevPos + inc, target);
        }

      target.pixelsChanged (boundRect);
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
  virtual void stamp (cVec2 pos, cTarget& target) {
  // stamp brushShape into image, clipped by width,height to pos, update mPrevPos

    int32_t width = target.getSize().x;
    int32_t height = target.getSize().y;

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
    uint8_t* frame = target.getPixels() + ((yFirst * width) + xFirst) * 4;
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
            *frame++ = (uint8_t)(r / 255);
            uint16_t g = (mG * foreground) + (*frame * background);
            *frame++ = (uint8_t)(g / 255);
            uint16_t b = (mB * foreground) + (*frame * background);
            *frame++ = (uint8_t)(b / 255);
            uint16_t a = (mA * foreground) + (*frame * background);
            *frame++ = (uint8_t)(a / 255);
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

  //{{{
  static cBrush* create (cGraphics& graphics, const std::string& className, float radius) {
    return new cPaintCpuBrush (graphics, className, radius);
    }
  //}}}
  inline static bool mRegistered = registerClass ("paintCpu", &create);
  };


// cPaintCpuShapeBrush - cpu stamp, precalc brush shape
class cPaintCpuShapeBrush : public cPaintCpuBrush {
public:
  //{{{
  cPaintCpuShapeBrush (cGraphics& graphics, const string& className, float radius)
      : cPaintCpuBrush (graphics, className, radius) {

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
  void listShape() {

    uint8_t* shapePtr = mShape;
    for (int y = 0; y < mShapeSize; y++) {
      string str;
      for (int x = 0; x < mShapeSize; x++)
        str += fmt::format ("{:3d} ", *shapePtr++);
      cLog::log (LOGINFO, str);
      }
    }
  //}}}
  //{{{
  void stamp (cVec2 pos, cTarget& target) final {
  // stamp brushShape into image, clipped by width,height to pos, update mPrevPos

    int32_t width = target.getSize().x;
    int32_t height = target.getSize().y;

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
    uint8_t* frame = target.getPixels() + ((yFirst * width) + xFirst) * 4;
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
            *frame++ = (uint8_t)(r / 255);
            uint16_t g = (mG * foreground) + (*frame * background);
            *frame++ = (uint8_t)(g / 255);
            uint16_t b = (mB * foreground) + (*frame * background);
            *frame++ = (uint8_t)(b / 255);
            uint16_t a = (mA * foreground) + (*frame * background);
            *frame++ = (uint8_t)(a / 255);
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

  //{{{
  static cBrush* create (cGraphics& graphics, const std::string& className, float radius) {
    return new cPaintCpuShapeBrush (graphics, className, radius);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("paintCpuShape", &create);
  };

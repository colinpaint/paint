// cPaintCpuShapeBrush.h
#pragma once
//{{{  includes
#include "cPaintCpuBrush.h"
//}}}

// cPaintCpuShapeBrush
class cPaintCpuShapeBrush : public cPaintCpuBrush {
public:
  cPaintCpuShapeBrush (const std::string& className, float radius);
  virtual ~cPaintCpuShapeBrush();

  void setRadius (float radius) final;

private:
  static cBrush* createBrush (const std::string& className, float radius) {
   return new cPaintCpuShapeBrush (className, radius);
    }

  void reportShape();

  void stamp (glm::vec2 pos, cFrameBuffer* frameBuffer) final;

  // vars
  int32_t mSubPixels = 4;
  float mSubPixelResolution = 0.f;

  uint8_t* mShape = nullptr;
  float mCreatedShapeRadius = 0.f;

  inline static const bool mRegistered = cBrushMan::registerClass ("paintCpuShape", &createBrush);
  };

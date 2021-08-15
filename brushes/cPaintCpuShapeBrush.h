// cPaintCpuShapeBrush.h
#pragma once
#include "cPaintCpuBrush.h"

// cPaintCpuShapeBrush
class cPaintCpuShapeBrush : public cPaintCpuBrush {
public:
  cPaintCpuShapeBrush (const std::string& className, float radius, cGraphics& graphics);
  virtual ~cPaintCpuShapeBrush();

  void setRadius (float radius) final;

private:
  static cBrush* createBrush (const std::string& className, float radius, cGraphics& graphics) {
    return new cPaintCpuShapeBrush (className, radius, graphics);
    }

  void reportShape();

  void stamp (glm::vec2 pos, cGraphics::cFrameBuffer* frameBuffer) final;

  // vars
  int32_t mSubPixels = 4;
  float mSubPixelResolution = 0.f;

  uint8_t* mShape = nullptr;
  float mCreatedShapeRadius = 0.f;

  inline static const bool mRegistered = registerClass ("paintCpuShape", &createBrush);
  };

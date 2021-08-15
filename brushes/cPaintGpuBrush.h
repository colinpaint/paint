// cPaintGpuBrush.h
#pragma once
#include "cBrush.h"
class cPaintShader;

// cPaintGpuBrush
class cPaintGpuBrush : public cBrush {
public:
  cPaintGpuBrush (const std::string& className, float radius, cGraphics& graphics);
  virtual ~cPaintGpuBrush();

  void paint (glm::vec2 pos, bool first, cGraphics::cFrameBuffer* frameBuffer, cGraphics::cFrameBuffer* frameBuffer1) final;

private:
  static cBrush* createBrush (const std::string& className, float radius, cGraphics& graphics) {
    return new cPaintGpuBrush (className, radius, graphics);
    }

  inline static const bool mRegistered = registerClass ("paintGpu", &createBrush);

  cGraphics::cPaintShader* mShader = nullptr;
  cGraphics& mGraphics;
  };

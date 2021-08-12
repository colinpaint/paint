// cPaintGpuBrush.h
#pragma once
//{{{  includes
#include "cBrushMan.h"
#include "cBrush.h"
class cPaintShader;
//}}}

// cPaintGpuBrush
class cPaintGpuBrush : public cBrush {
public:
  cPaintGpuBrush (const std::string& className, float radius);
  virtual ~cPaintGpuBrush();

  void paint (glm::vec2 pos, bool first, cFrameBuffer* frameBuffer, cFrameBuffer* frameBuffer1) final;

private:
  static cBrush* createBrush (const std::string& className, float radius) {
    return new cPaintGpuBrush (className, radius);
    }

  inline static const bool mRegistered = cBrushMan::registerClass ("paintGpu", &createBrush);

  cPaintShader* mShader = nullptr;
  };

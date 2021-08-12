// cPaintCpuBrush.h
#pragma once
//{{{  includes
#include "cBrushMan.h"
#include "cBrush.h"
//}}}

// cPaintCpuBrush
class cPaintCpuBrush : public cBrush {
public:
  cPaintCpuBrush (const std::string& className, float radius);
  virtual ~cPaintCpuBrush() = default;

  virtual void setRadius (float radius) override;
  void paint (glm::vec2 pos, bool first, cFrameBuffer* frameBuffer, cFrameBuffer* frameBuffer1) final;

protected:
  uint8_t getPaintShape (float i, float j, float radius);

  int32_t mShapeRadius = 0;
  int32_t mShapeSize = 0;

private:
  static cBrush* createBrush (const std::string& className, float radius) {
    return new cPaintCpuBrush (className, radius);
    }

  virtual void stamp (glm::vec2 pos, cFrameBuffer* frameBuffer);

  inline static const bool mRegistered = cBrushMan::registerClass ("paintCpu", &createBrush);
  };

// cBrush.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// glm
#include <vec2.hpp>
#include <vec4.hpp>

#include "../graphics/cPointRect.h"
class cFrameBuffer;
//}}}

// cBrush
class cBrush {
public:
  cBrush (const std::string& name, float radius);
  virtual ~cBrush() = default;

  std::string getName() const { return mName; }
  glm::vec4 getColor();

  float getRadius() { return mRadius; }
  float getBoundRadius() { return mRadius + 1.f; }
  cRect getBoundRect (glm::vec2 pos, cFrameBuffer* frameBuffer);

  void setColor (glm::vec4 color);
  void setColor (float r, float g, float b, float a);
  void setColor (uint8_t r, uint8_t g, uint8_t b, uint8_t a);

  // virtuals
  virtual void setRadius (float radius) { mRadius = radius; }
  virtual void paint (glm::vec2 pos, bool first, cFrameBuffer* frameBuffer, cFrameBuffer* frameBuffer1) = 0;

protected:
  float mRadius = 0.f;

  uint8_t mR = 0;
  uint8_t mG = 0;
  uint8_t mB = 0;
  uint8_t mA = 255;

  glm::vec2 mPrevPos = glm::vec2(0.f,0.f);

private:
  const std::string mName;
  };

// cOpenGlGraphics.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

// glm
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>

#include "cGraphics.h"
//}}}

class cOpenGlGraphics : public cGraphics {
public:
  cOpenGlGraphics() = default;
  virtual ~cOpenGlGraphics() = default;

  bool init (void* device, void* deviceContext, void* swapChain);
  void shutdown();

  // create graphics resources
  cQuad* createQuad (cPoint size);
  cQuad* createQuad (cPoint size, const cRect& rect);

  cFrameBuffer* createFrameBuffer();
  cFrameBuffer* createFrameBuffer (cPoint size, cFrameBuffer::eFormat format);
  cFrameBuffer* createFrameBuffer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format);

  cCanvasShader* createCanvasShader();
  cLayerShader* createLayerShader();
  cPaintShader* createPaintShader();

  // actions
  void draw();
  void windowResized (int width, int height);
  };

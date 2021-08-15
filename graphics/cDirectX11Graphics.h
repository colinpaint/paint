// cDirectX11Graphics.h - concrete DirectX11 graphic class
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

class cDirectX11Graphics : public cGraphics {
public:
  bool init (void* device, void* deviceContext, void* swapChain) final;
  void shutdown() final;

  // create resources
  cQuad* createQuad (cPoint size) final;
  cQuad* createQuad (cPoint size, const cRect& rect) final;

  cFrameBuffer* createFrameBuffer() final;
  cFrameBuffer* createFrameBuffer (cPoint size, cFrameBuffer::eFormat format) final;
  cFrameBuffer* createFrameBuffer (uint8_t* pixels, cPoint size, cFrameBuffer::eFormat format) final;

  cCanvasShader* createCanvasShader() final;
  cLayerShader* createLayerShader() final;
  cPaintShader* createPaintShader() final;

  // actions
  void draw() final;
  void windowResized (int width, int height) final;
  };

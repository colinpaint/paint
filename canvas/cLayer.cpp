// cLayer.cpp
//{{{  includes
#include "cLayer.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// glm
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>
#include <gtc/matrix_transform.hpp>

#include "../graphics/cGraphics.h"
#include "../brushes/cBrush.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  cGraphics::cLayerShader* shader = nullptr;
  }

//{{{
cLayer::cLayer (cPoint size, cGraphics::cFrameBuffer::eFormat format, cGraphics& graphics)
    : mSize(size), mFormat(format), mGraphics(graphics) {

  // allocate and clear mPixels
  int numBytes = size.x * size.y * 4;
  auto data = static_cast<uint8_t*>(malloc (numBytes));
  memset (data, 0, numBytes);
  mFrameBuffer = graphics.createFrameBuffer (data, size, format);
  free (data);

  mFrameBuffer1 = graphics.createFrameBuffer (size, format);

  mQuad = graphics.createQuad (size);
  }
//}}}
//{{{
cLayer::cLayer (uint8_t* data, cPoint size, cGraphics::cFrameBuffer::eFormat format, cGraphics& graphics)
    : mSize(size), mFormat(format), mGraphics(graphics) {

  mFrameBuffer = graphics.createFrameBuffer (data, size, format);
  mFrameBuffer1 = graphics.createFrameBuffer (size, format);

  mQuad = graphics.createQuad (size);
  }
//}}}
//{{{
cLayer::~cLayer() {

  delete mFrameBuffer;
  delete mFrameBuffer1;
  }
//}}}

//{{{
uint32_t cLayer::getTextureId() {
  return mFrameBuffer->getTextureId();
  }
//}}}
//{{{
uint8_t* cLayer::getPixels() {
  return mFrameBuffer->getPixels();
  }
//}}}
//{{{
uint8_t* cLayer::getRenderedPixels() {
// draw layer processsed -> frameBuffer1

  mFrameBuffer1->setTarget();
  mFrameBuffer1->setBlend();
  mFrameBuffer1->invalidate();

  draw (mSize);

  return mFrameBuffer1->getPixels();
  }
//}}}

//{{{
void cLayer::paint (cBrush* brush, glm::vec2 pos, bool first) {
// paint using brush onto layer's frameBuffers

  brush->paint (pos, first, mFrameBuffer, mFrameBuffer1);
  }
//}}}
//{{{
void cLayer::draw (const cPoint& size) {
// draw layer to current target:size

  if (mVisible) {
    mFrameBuffer->setSource();
    mFrameBuffer->checkStatus();

    if (!shader)
      shader = mGraphics.createLayerShader();

    shader->use();
    shader->setModelProject (
      glm::translate (glm::mat4 (1.f), glm::vec3 ((size.x - mSize.x)/2.f, (size.y - mSize.y)/2.f, 0.f)),
      glm::ortho (0.f,static_cast<float>(size.x), 0.f,static_cast<float>(size.y), -1.f,1.f));
    shader->setHueSatVal (0.f, 0.f, 0.f);

    mQuad->draw();
    }
  }
//}}}

//{{{
void cLayer::replace (uint8_t* pixels) {

  //free (mPixels);
  //mPixels = pixels;
  //update (0,0, mWidth,mHeight);
  }
//}}}

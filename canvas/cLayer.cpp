// cLayer.cpp
//{{{  includes
#include "cLayer.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include "../graphics/cGraphics.h"
#include "../brush/cBrush.h"
#include "../utils/cLog.h"

using namespace std;
using namespace fmt;
//}}}

namespace {
  cLayerShader* gShader = nullptr;
  }

//{{{
cLayer::cLayer (cPoint size, cFrameBuffer::eFormat format, cGraphics& graphics)
    : mSize(size), mFormat(format), mGraphics(graphics) {

  // allocate and clear mPixels
  int numBytes = size.x * size.y * 4;
  auto data = static_cast<uint8_t*>(malloc (numBytes));
  memset (data, 0, numBytes);
  mFrameBuffer = graphics.createFrameBuffer (data, size, format);
  if (!mFrameBuffer)
     cLog::log (LOGERROR, "createFrameBuffer failed");
  free (data);

  mFrameBuffer1 = graphics.createFrameBuffer (size, format);
  if (!mFrameBuffer1)
    cLog::log (LOGERROR, "createFrameBuffer failed");

  mQuad = graphics.createQuad (size);
  if (!mQuad)
    cLog::log (LOGERROR, "createQuad failed");
  }
//}}}
//{{{
cLayer::cLayer (uint8_t* data, cPoint size, cFrameBuffer::eFormat format, cGraphics& graphics)
    : mSize(size), mFormat(format), mGraphics(graphics) {

  mFrameBuffer = graphics.createFrameBuffer (data, size, format);
  if (!mFrameBuffer)
     cLog::log (LOGERROR, "createFrameBuffer failed");

  mFrameBuffer1 = graphics.createFrameBuffer (size, format);
  if (!mFrameBuffer1)
    cLog::log (LOGERROR, "createFrameBuffer failed");

  mQuad = graphics.createQuad (size);
  if (!mQuad)
    cLog::log (LOGERROR, "createQuad failed");
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
void cLayer::paint (cBrush* brush, cVec2 pos, bool first) {
// paint using brush onto layer's frameBuffers

  brush->paint (pos, first, *mFrameBuffer, *mFrameBuffer1);
  }
//}}}
//{{{
void cLayer::draw (const cPoint& size) {
// draw layer to current target:size

  if (mVisible) {
    mFrameBuffer->setSource();
    mFrameBuffer->checkStatus();

    if (!gShader) {
      gShader = mGraphics.createLayerShader();
      if (!gShader)
        cLog::log (LOGERROR, "createLayerShader failed");
      }

    gShader->use();

    cMat4x4 model;
    model.translate (cVec3 ((size.x - mSize.x)/2.f, (size.y - mSize.y)/2.f, 0.f));
    cMat4x4 project;
    project.setOrtho (0.f,static_cast<float>(size.x) , 0.f, static_cast<float>(size.y), -1.f, 1.f);
    gShader->setModelProject (model, project);
    gShader->setHueSatVal (0.f, 0.f, 0.f);

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

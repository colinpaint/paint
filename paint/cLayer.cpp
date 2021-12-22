// cLayer.cpp
//{{{  includes
#include "cLayer.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include "../app/cGraphics.h"
#include "../brush/cBrush.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

namespace {
  cLayerShader* gShader = nullptr;
  }

//{{{
cLayer::cLayer (cPoint size, cTarget::eFormat format, cGraphics& graphics)
    : mSize(size), mFormat(format), mGraphics(graphics) {

  // allocate and clear mPixels
  int numBytes = size.x * size.y * 4;
  auto data = static_cast<uint8_t*>(malloc (numBytes));
  memset (data, 0, numBytes);
  mTarget = graphics.createTarget (data, size, format);
  if (!mTarget)
     cLog::log (LOGERROR, "createTarget failed");
  free (data);

  mTarget1 = graphics.createTarget (size, format);
  if (!mTarget1)
    cLog::log (LOGERROR, "createTarget failed");

  mQuad = graphics.createQuad (size);
  if (!mQuad)
    cLog::log (LOGERROR, "createQuad failed");
  }
//}}}
//{{{
cLayer::cLayer (uint8_t* data, cPoint size, cTarget::eFormat format, cGraphics& graphics)
    : mSize(size), mFormat(format), mGraphics(graphics) {

  mTarget = graphics.createTarget (data, size, format);
  if (!mTarget)
     cLog::log (LOGERROR, "createTarget failed");

  mTarget1 = graphics.createTarget (size, format);
  if (!mTarget1)
    cLog::log (LOGERROR, "createTarget failed");

  mQuad = graphics.createQuad (size);
  if (!mQuad)
    cLog::log (LOGERROR, "createQuad failed");
  }
//}}}
//{{{
cLayer::~cLayer() {

  delete mTarget;
  delete mTarget1;
  }
//}}}

//{{{
uint32_t cLayer::getTextureId() {
  return mTarget->getTextureId();
  }
//}}}
//{{{
uint8_t* cLayer::getPixels() {
  return mTarget->getPixels();
  }
//}}}
//{{{
uint8_t* cLayer::getRenderedPixels() {
// draw layer processsed -> Target1

  mTarget1->setTarget();
  mTarget1->setBlend();
  mTarget1->invalidate();

  draw (mSize);

  return mTarget1->getPixels();
  }
//}}}

//{{{
void cLayer::paint (cBrush* brush, cVec2 pos, bool first) {
// paint using brush onto layer's Targets

  brush->paint (pos, first, *mTarget, *mTarget1);
  }
//}}}
//{{{
void cLayer::draw (const cPoint& size) {
// draw layer to current target:size

  if (mVisible) {
    mTarget->setSource();
    mTarget->checkStatus();

    if (!gShader) {
      gShader = mGraphics.createLayerShader();
      if (!gShader)
        cLog::log (LOGERROR, "createLayerShader failed");
      }

    gShader->use();

    cMat4x4 model;
    model.setTranslate (cVec3 ((size.x - mSize.x)/2.f, (size.y - mSize.y)/2.f, 0.f));
    cMat4x4 orthoProjection (0.f,static_cast<float>(size.x) , 0.f, static_cast<float>(size.y), -1.f, 1.f);
    gShader->setModelProjection (model, orthoProjection);

    gShader->setHueSatVal (0.f, 0.f, 0.f);

    mQuad->draw();
    }
  }
//}}}

//{{{
void cLayer::replace (uint8_t* pixels) {

  (void)pixels;
  //free (mPixels);
  //mPixels = pixels;
  //update (0,0, mWidth,mHeight);
  }
//}}}

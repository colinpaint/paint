// cCanvas.cpp - layers container
//{{{  includes
#include "cCanvas.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// glm
#include <vec2.hpp>
#include <vec3.hpp>
#include <vec4.hpp>
#include <mat4x4.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
#include <gtc/matrix_inverse.hpp>
#include <gtc/matrix_transform.hpp>

// stb
#include <stb_image.h>
#include <stb_image_write.h>

#include "../brushes/cBrush.h"
#include "../graphics/cGraphics.h"
#include "../log/cLog.h"

#include "cLayer.h"

using namespace std;
using namespace fmt;
//}}}

//{{{
cCanvas::cCanvas (cPoint size, cGraphics& graphics) : mSize(size), mNumChannels(4), mGraphics(graphics) {

  // create empty layer
  mLayers.push_back (new cLayer (mSize, cFrameBuffer::eRGBA, graphics));
  createResources();
  }
//}}}
//{{{
cCanvas::cCanvas (const string& fileName, cGraphics& graphics) : mName(fileName), mGraphics(graphics) {

  // load file image
  uint8_t* pixels = stbi_load (fileName.c_str(), &mSize.x, &mSize.y, &mNumChannels, 4);
  cLayer* layer = new cLayer (pixels, mSize, cFrameBuffer::eRGBA, graphics);
  free (pixels);

  layer->setName (fileName);
  mLayers.push_back (layer);

  cLog::log (LOGINFO, format ("new canvas - {} {} {} {}", fileName, mSize.x, mSize.y, mNumChannels));

  createResources();
  }
//}}}
//{{{
cCanvas::~cCanvas() {

  // delete resources
  for (auto layer : mLayers)
    delete layer;

  delete mWindowFrameBuffer;
  delete mFrameBuffer;
  delete mShader;
  delete mQuad;
  }
//}}}

// gets
//{{{
uint8_t* cCanvas::getPixels (cPoint& size) {

  size = mFrameBuffer->getSize();
  return mFrameBuffer->getPixels();
  }
//}}}

// layers
//{{{
unsigned cCanvas::newLayer() {
  mLayers.push_back (new cLayer (mSize, cFrameBuffer::eRGBA, mGraphics));
  return static_cast<unsigned>(mLayers.size() - 1);
  }
//}}}
//{{{
unsigned cCanvas::newLayer (const string& fileName) {

  cPoint size;
  int numChannels;
  uint8_t* pixels = stbi_load (fileName.c_str(), &size.x, &size.y, &numChannels, 0);

  cLog::log (LOGINFO, format ("new layer {} {},{} {}", fileName, size.x, size.y, numChannels));

  // new layer, transfer ownership of pixels to texture
  mLayers.push_back (new cLayer (pixels, size, cFrameBuffer::eRGBA, mGraphics));
  return static_cast<unsigned>(mLayers.size() - 1);
}
//}}}
//{{{
void cCanvas::switchCurLayer (unsigned layerIndex) {

  if (layerIndex == mCurLayerIndex)
    return;
  if (layerIndex >= mLayers.size())
    return;

  mCurLayerIndex = layerIndex;
  }
//}}}
//{{{
void cCanvas::deleteLayer (unsigned layerIndex) {

  // Must have at least 1 layer.
  if (mLayers.size() == 1)
    return;

  if (layerIndex < mCurLayerIndex)
    mCurLayerIndex -= 1;
  else if (mCurLayerIndex == layerIndex) {
    if (mCurLayerIndex == mLayers.size() - 1)
      mCurLayerIndex -= 1;
    }

  delete mLayers[layerIndex];
  mLayers.erase (mLayers.begin() + layerIndex);
  }
//}}}

//{{{
void cCanvas::renderCurLayer() {

  uint8_t* pixels = getCurLayer()->getRenderedPixels();
  getCurLayer()->replace (pixels);
  }
//}}}

//{{{
void cCanvas::mouse (bool active, bool clicked, bool dragging, bool released, glm::vec2 pos, glm::vec2 drag) {

  // just paint for now, will expand to other actions, graphics,cut,paste,effects
  if (active)
    getCurLayer()->paint (cBrush::getCurBrush(), getLayerPos (pos), clicked);
  }
//}}}
//{{{
void cCanvas::draw (cPoint windowSize) {

  // draw layers -> canvas frameBuffer
  mFrameBuffer->setTarget();
  mFrameBuffer->invalidate();
  mFrameBuffer->setBlend();

  for (auto layer : mLayers)
    layer->draw (mSize);

  // draw canvas frameBuffer -> window frameBuffer
  mWindowFrameBuffer->setSize (windowSize);
  mWindowFrameBuffer->setTarget();
  mWindowFrameBuffer->setBlend();
  mWindowFrameBuffer->clear (glm::vec4(0.25f,0.25f,0.25f,1.0f));

  // - canvas frameBuffer texture source
  mFrameBuffer->setSource();
  mFrameBuffer->checkStatus();

  // - shader
  mShader->use();
  mShader->setModelProject (
    glm::translate (glm::mat4 (1.f), glm::vec3 ((windowSize.x - mSize.x)/2.f, (windowSize.y - mSize.y)/2.f, 0.f)),
    glm::ortho (0.f,static_cast<float>(windowSize.x), 0.f, static_cast<float>(windowSize.y), -1.f,1.f));

  mQuad->draw();
  }
//}}}

// private:
//{{{
void cCanvas::createResources() {

  // create quad
  mQuad = mGraphics.createQuad (mSize);

  // create canvasShader
  mShader = mGraphics.createCanvasShader();

  // create target
  mFrameBuffer = mGraphics.createFrameBuffer (nullptr, mSize, cFrameBuffer::eRGBA);

  // create window
  mWindowFrameBuffer = mGraphics.createFrameBuffer();

  // select brush
  cBrush::setCurBrushByName ("paintGpu", 20.f, mGraphics);
  }
//}}}
//{{{
glm::vec2 cCanvas::getLayerPos (glm::vec2 pos) {

  // need a better calc, not sure mModel and mProject help
  //cLog::log (LOGINFO, "mod " + glm::to_string (mModel));
  //cLog::log (LOGINFO, "prj " + glm::to_string (mProject));
  pos.x = (mSize.x/2.f) + pos.x;
  pos.y = (mSize.y/2.f) + pos.y;

  return pos;
  };
//}}}

// cPaintApp.cpp - layers container
//{{{  includes
#include "cPaintApp.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// stb
#include <stb_image.h>
#include <stb_image_write.h>

#include "../brush/cBrush.h"
#include "../app/cGraphics.h"
#include "../utils/cLog.h"

#include "cLayer.h"

using namespace std;
//}}}

//{{{
cPaintApp::cPaintApp (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont, cPoint size)
   : cApp (platform, graphics, mainFont, monoFont), mSize(size), mNumChannels(4) {

  // create empty layer
  mLayers.push_back (new cLayer (mSize, cFrameBuffer::eRGBA, graphics));
  createResources();
  }
//}}}
//{{{
cPaintApp::cPaintApp (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont, const std::string& filename)
    : cApp (platform, graphics, mainFont, monoFont) {

  mFilename = filename;

  // load file image
  uint8_t* pixels = stbi_load (filename.c_str(), &mSize.x, &mSize.y, &mNumChannels, 4);
  cLayer* layer = new cLayer (pixels, mSize, cFrameBuffer::eRGBA, getGraphics());
  free (pixels);

  layer->setName (filename);
  mLayers.push_back (layer);

  cLog::log (LOGINFO, fmt::format ("new canvas - {} {} {} {}", filename, mSize.x, mSize.y, mNumChannels));

  createResources();
  }
//}}}
//{{{
cPaintApp::~cPaintApp() {

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
uint8_t* cPaintApp::getPixels (cPoint& size) {

  size = mFrameBuffer->getSize();
  return mFrameBuffer->getPixels();
  }
//}}}

// layers
//{{{
unsigned cPaintApp::newLayer() {
  mLayers.push_back (new cLayer (mSize, cFrameBuffer::eRGBA, getGraphics()));
  return static_cast<unsigned>(mLayers.size() - 1);
  }
//}}}
//{{{
unsigned cPaintApp::newLayer (const string& fileName) {

  cPoint size;
  int numChannels;
  uint8_t* pixels = stbi_load (fileName.c_str(), &size.x, &size.y, &numChannels, 0);

  cLog::log (LOGINFO, fmt::format ("new layer {} {},{} {}", fileName, size.x, size.y, numChannels));

  // new layer, transfer ownership of pixels to texture
  mLayers.push_back (new cLayer (pixels, size, cFrameBuffer::eRGBA, getGraphics()));
  return static_cast<unsigned>(mLayers.size() - 1);
}
//}}}
//{{{
void cPaintApp::switchCurLayer (unsigned layerIndex) {

  if (layerIndex == mCurLayerIndex)
    return;
  if (layerIndex >= mLayers.size())
    return;

  mCurLayerIndex = layerIndex;
  }
//}}}
//{{{
void cPaintApp::deleteLayer (unsigned layerIndex) {

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
void cPaintApp::renderCurLayer() {

  uint8_t* pixels = getCurLayer()->getRenderedPixels();
  getCurLayer()->replace (pixels);
  }
//}}}

//{{{
void cPaintApp::mouse (bool active, bool clicked, bool dragging, bool released, cVec2 pos, cVec2 drag) {

  (void)dragging;
  (void)released;
  (void)drag;
  // just paint for now, will expand to other actions, graphics,cut,paste,effects
  if (active)
    getCurLayer()->paint (cBrush::getCurBrush(), getLayerPos (pos), clicked);
  }
//}}}
//{{{
void cPaintApp::draw (cPoint windowSize) {

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
  mWindowFrameBuffer->clear (cColor(0.25f,0.25f,0.25f,1.0f));

  // - canvas frameBuffer texture source
  mFrameBuffer->setSource();
  mFrameBuffer->checkStatus();

  // - shader
  mShader->use();

  cMat4x4 model;
  model.setTranslate (cVec2 ((windowSize.x - mSize.x)/2.f, (windowSize.y - mSize.y)/2.f));
  cMat4x4 orthoProjection (0.f,static_cast<float>(windowSize.x) , 0.f, static_cast<float>(windowSize.y), -1.f, 1.f);
  mShader->setModelProjection (model, orthoProjection);

  mQuad->draw();
  }
//}}}

// private:
//{{{
void cPaintApp::createResources() {

  // create quad
  mQuad = getGraphics().createQuad (mSize);

  // create canvasShader
  mShader = getGraphics().createTextureShader (cTexture::eRgba);

  // create target
  mFrameBuffer = getGraphics().createFrameBuffer (nullptr, mSize, cFrameBuffer::eRGBA);

  // create window
  mWindowFrameBuffer = getGraphics().createFrameBuffer();

  // select brush
  cBrush::setCurBrushByName (getGraphics(), "paintGpu", 20.f);
  }
//}}}
//{{{
cVec2 cPaintApp::getLayerPos (cVec2 pos) {

  // need a better calc, not sure mModel and mProject help
  //cLog::log (LOGINFO, "mod " + glm::to_string (mModel));
  //cLog::log (LOGINFO, "prj " + glm::to_string (mProject));
  pos.x = (mSize.x/2.f) + pos.x;
  pos.y = (mSize.y/2.f) + pos.y;

  return pos;
  }
//}}}

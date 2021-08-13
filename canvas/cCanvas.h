// cCanvas.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <vector>

// glm
#include <vec2.hpp>
#include <mat4x4.hpp>

#include "../graphics/cPointRect.h"

class cQuad;
class cCanvasShader;
class cFrameBuffer;
class cLayer;
class cBrush;
//}}}

class cCanvas {
public:
  cCanvas (cPoint size);
  cCanvas (const std::string& fileName);
  ~cCanvas();

  // gets
  std::string getName() { return mName; }
  cPoint getSize() { return mSize; }

  size_t getNumLayers() { return mLayers.size(); }
  std::vector<cLayer*>& getLayers() { return mLayers; }

  unsigned getCurLayerIndex() { return mCurLayerIndex; }
  bool isCurLayer (unsigned layerIndex) { return layerIndex == mCurLayerIndex; }

  cLayer* getLayer (unsigned layerIndex) { return mLayers[layerIndex]; }
  cLayer* getCurLayer() { return getLayer (mCurLayerIndex); }

  uint8_t* getPixels (cPoint& size);

  // set
  void setName (const std::string name) { mName = name; }

  // layers
  unsigned newLayer();
  unsigned newLayer (const std::string& fileName);
  void toggleLayerVisible (unsigned layerIndex);
  void switchCurLayer (unsigned layerIndex);
  void deleteLayer (unsigned layerIndex);

  void renderCurLayer();

  void mouse (bool active, bool clicked, bool dragging, bool released, glm::vec2 pos, glm::vec2 drag);
  void draw (cPoint windowSize);
  void drawNothing (cPoint windowSize);

private:
  void createResources();
  glm::vec2 getLayerPos (glm::vec2 pos);

  // vars
  std::string mName;

  cPoint mSize;
  int mNumChannels = 0;

  unsigned mCurLayerIndex = 0;
  std::vector<cLayer*> mLayers;

  float mHue = 0.f;
  float mSat = 0.f;
  float mVal = 0.f;

  cQuad* mQuad = nullptr;
  cCanvasShader* mShader = nullptr;
  cFrameBuffer* mFrameBuffer = nullptr;
  cFrameBuffer* mWindowFrameBuffer = nullptr;
  };

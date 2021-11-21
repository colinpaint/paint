// cCanvas.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>
#include <vector>

#include "../utils/cApp.h"
#include "../graphics/cGraphics.h"

class cPlatform;
class cLayer;
class cBrush;
//}}}

class cCanvas : public cApp {
public:
  cCanvas (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont, cPoint size);
  cCanvas (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont, const std::string& filename);
  virtual ~cCanvas();

  // gets
  std::string getName() const { return mFilename; };

  cPoint getSize() { return mSize; }

  size_t getNumLayers() { return mLayers.size(); }
  std::vector<cLayer*>& getLayers() { return mLayers; }

  unsigned getCurLayerIndex() { return mCurLayerIndex; }
  bool isCurLayer (unsigned layerIndex) { return layerIndex == mCurLayerIndex; }

  cLayer* getLayer (unsigned layerIndex) { return mLayers[layerIndex]; }
  cLayer* getCurLayer() { return getLayer (mCurLayerIndex); }

  uint8_t* getPixels (cPoint& size);


  // layers
  unsigned newLayer();
  unsigned newLayer (const std::string& fileName);
  void toggleLayerVisible (unsigned layerIndex);
  void switchCurLayer (unsigned layerIndex);
  void deleteLayer (unsigned layerIndex);

  void renderCurLayer();

  void mouse (bool active, bool clicked, bool dragging, bool released, cVec2 pos, cVec2 drag);
  void draw (cPoint windowSize);

private:
  void createResources();
  cVec2 getLayerPos (cVec2 pos);

  std::string mFilename;
  cPoint mSize;
  int mNumChannels = 0;

  unsigned mCurLayerIndex = 0;
  std::vector<cLayer*> mLayers;

  float mHue = 0.f;
  float mSat = 0.f;
  float mVal = 0.f;

  cQuad* mQuad = nullptr;
  cTextureShader* mShader = nullptr;
  cFrameBuffer* mFrameBuffer = nullptr;
  cFrameBuffer* mWindowFrameBuffer = nullptr;
  };

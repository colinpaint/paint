// cVideoRender.cpp
//{{{  includes
#include "cVideoRender.h"
#include "cVideoFrame.h"

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <functional>

#include "../common/cDvbUtils.h"
#include "../app/cGraphics.h"

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/utils.h"

//{{{  include libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/motion_vector.h>
  #include <libavutil/frame.h>
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}
#include "cFFmpegVideoFrame.h"
#include "cFFmpegVideoDecoder.h"

using namespace std;
//}}}
constexpr bool kVideoQueued = true;
constexpr size_t kVideoFrameMapSize = 25;

namespace {
  cTextureShader* gShader = nullptr;
  }

// cVideoRender
//{{{
cVideoRender::cVideoRender (const string& name, uint8_t streamType, bool realTime)
    : cRender(kVideoQueued, name + "vid", streamType, kVideoFrameMapSize, realTime) {

  mDecoder = new cFFmpegVideoDecoder (*this, streamType);
  setAllocFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });
  }
//}}}
//{{{
cVideoRender::~cVideoRender() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFrames)
    delete frame.second;
  mFrames.clear();

  for (auto& frame : mFreeFrames)
    delete frame;

  mFreeFrames.clear();
  }
//}}}

//{{{
cVideoFrame* cVideoRender::getVideoFrameFromPts (int64_t pts) {

  if (mFrames.empty() || !mPtsDuration)
    return nullptr;
  else
    return dynamic_cast<cVideoFrame*>(getFrameFromPts (pts / mPtsDuration));
  }
//}}}
//{{{
cVideoFrame* cVideoRender::getVideoNearestFrameFromPts (int64_t pts) {

  if (mFrames.empty() || !mPtsDuration)
    return nullptr;
   else
    return dynamic_cast<cVideoFrame*>(getNearestFrameFromPts (pts / mPtsDuration));
  }
//}}}

// decoder callbacks
//{{{
cFrame* cVideoRender::getFrame() {

  cFrame* frame = getFreeFrame();
  if (frame)
    return frame;
  else
    return new cFFmpegVideoFrame();
  }
//}}}
//{{{
void cVideoRender::addFrame (cFrame* frame) {

  cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);

  videoFrame->mQueueSize = getQueueSize();
  videoFrame->mTextureDirty = true;

  // save some videoFrame info
  mWidth = videoFrame->mWidth;
  mHeight = videoFrame->mHeight;
  mPtsDuration = videoFrame->mPtsDuration;
  mFrameInfo = videoFrame->getInfoString();

  { // locked
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFrames.emplace (videoFrame->mPts / videoFrame->mPtsDuration, videoFrame);
  }

  }
//}}}

// UI draw
//{{{
cRect cVideoRender::drawFrame (cVideoFrame* videoFrame, cGraphics& graphics, cMat4x4& model,
                               float viewportWidth, float viewportHeight) {

  // get texture
  cTexture& texture = videoFrame->getTexture (graphics);
  texture.setSource();

  // ensure shader is created
  if (!gShader)
    gShader = graphics.createTextureShader (texture.getTextureType());
  gShader->use();

  cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);
  gShader->setModelProjection (model, projection);

  // ensure quad is created
  if (!mQuad)
    mQuad = graphics.createQuad (videoFrame->getSize());
  mQuad->draw();

  return cRect (model.transform (cVec2(0, videoFrame->getHeight()), viewportHeight),
                model.transform (cVec2(videoFrame->getWidth(), 0), viewportHeight));
  }
//}}}

// overrides
//{{{
string cVideoRender::getInfoString() const {
  return fmt::format ("vid frames:{:2d}:{:2d}:{:d} {} {}",
                      mFrames.size(), mFreeFrames.size(), getQueueSize(),
                      mDecoder->getInfoString(), mFrameInfo);
  }
//}}}
//{{{
void cVideoRender::trimVideoBeforePts (int64_t pts) {
  trimFramesBeforePts (pts / mPtsDuration);
  }
//}}}

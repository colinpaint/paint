// cRender.cpp - dvb stream render base class
//{{{  includes
#include "cRender.h"

#include <cstdint>
#include <string>
#include <map>
#include <thread>

#include "../common/cLog.h"

using namespace std;
//}}}
constexpr int64_t kPtsPerFrame = 90000 / 25;

// public:
//{{{
cRender::cRender (bool queued, const string& name, uint8_t streamTypeId, uint16_t decoderMask, size_t frameMapSize)
    : mFrameMapSize(frameMapSize), mQueued(queued),
      mName(name), mStreamTypeId(streamTypeId), mDecoderMask(decoderMask), mMiniLog ("log") {

  if (queued)
    thread ([=](){ startQueueThread (name + "Q"); }).detach();
  }
//}}}
//{{{
cRender::~cRender() {
  stopQueueThread();
  }
//}}}

//{{{
float cRender::getValue (int64_t pts) const {

  auto it = mValuesMap.find (pts / kPtsPerFrame);
  return it == mValuesMap.end() ? 0.f : it->second;
  }
//}}}
//{{{
float cRender::getOffsetValue (int64_t ptsOffset, int64_t& pts) const {

  pts = mRefPts - ptsOffset;
  auto it = mValuesMap.find (pts / kPtsPerFrame);
  return it == mValuesMap.end() ? 0.f : it->second;
  }
//}}}

// frames, freeFrames
//{{{
cFrame* cRender::getFramePts (int64_t pts) {

  // unlocked find
  auto it = mFrames.find (pts);
  return (it == mFrames.end()) ? nullptr : it->second;
  }
//}}}
//{{{
cFrame* cRender::getFreeFrame() {

  // quick unlocked test for none
  if (mFreeFrames.empty())
    return nullptr;

  // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  cFrame* frame = mFreeFrames.front();
  mFreeFrames.pop_front();
  return frame;
  }
//}}}
//{{{
cFrame* cRender::getYoungestFrame() {

  // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  // reuse youngest
  auto it = mFrames.begin();
  cFrame* youngestFrame = (*it).second;
  mFrames.erase (it);

  youngestFrame->releaseResources();
  return youngestFrame;
  }
//}}}
//{{{
void cRender::trimFramesBeforePts (int64_t pts) {
// remove frames before pts, release any temp resources

  // quick test, unlocked
  if (mFrames.empty())
    return;

  // locked
  unique_lock<shared_mutex> lock(mSharedMutex);

  auto it = mFrames.begin();
  while ((it != mFrames.end()) && ((*it).first < pts)) {
    it->second->releaseResources();
    mFreeFrames.push_back (it->second);
    it = mFrames.erase (it);
    }
  }
//}}}

// log
void cRender::toggleLog() { mMiniLog.toggleEnable(); }
void cRender::header() { mMiniLog.setHeader (fmt::format ("header")); }
void cRender::log (const string& tag, const string& text) { mMiniLog.log (tag, text); }
//{{{
void cRender::logValue (int64_t pts, float value) {

  while (mValuesMap.size() >= mMapSize)
    mValuesMap.erase (mValuesMap.begin());

  mValuesMap.emplace (pts / kPtsPerFrame, value);

  if (pts > mLastPts)
    mLastPts = pts;
  }
//}}}

/// decode queue
//{{{
size_t cRender::getQueueSize() const {
  return mDecodeQueue.size_approx();
  }
//}}}
//{{{
float cRender::getQueueFrac() const {
  return (float)mDecodeQueue.size_approx() / mDecodeQueue.max_capacity();
  }
//}}}

// process
//{{{
bool cRender::processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {

  (void)skip;
  //log ("pes", fmt::format ("pts:{} size:{}", getFullPtsString (pts), pesSize));
  //logValue (pts, (float)pesSize);

  if (isQueued()) {
    mDecodeQueue.enqueue (new cDecodeQueueItem (mDecoder, pes, pesSize, pts, dts, mAllocFrameCallback, mAddFrameCallback));
    return true;
    }
  else {
    mDecoder->decode (pes, pesSize, pts, dts, mAllocFrameCallback, mAddFrameCallback);
    return false;
    }
  }
//}}}

// private:
//{{{
void cRender::startQueueThread (const string& name) {

  cLog::setThreadName (name);

  mQueueRunning = true;

  while (!mQueueExit) {
    cDecodeQueueItem* queueItem;
    if (mDecodeQueue.wait_dequeue_timed (queueItem, 40000)) {
      queueItem->mDecoder->decode (queueItem->mPes, queueItem->mPesSize, queueItem->mPts, queueItem->mDts,
                                   queueItem->mAllocFrameCallback, queueItem->mAddFrameCallback);
      delete queueItem;
      }
    }

  if (mDecodeQueue.size_approx())
    cLog::log (LOGINFO, fmt::format ("startQueueThread - should empty queue {}", mDecodeQueue.size_approx()));

  mQueueRunning = false;
  }
//}}}
//{{{
void cRender::stopQueueThread() {

  if (mQueued) {
    mQueueExit = true;
    while (mQueueRunning)
      this_thread::sleep_for (100ms);
    }
  }
//}}}

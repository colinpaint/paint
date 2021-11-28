// cRender.cpp - dvb stream render base class
//{{{  includes
#include "cRender.h"

#include <cstdint>
#include <string>
#include <map>
#include <thread>

#include "../utils/cLog.h"

using namespace std;
//}}}
constexpr int64_t kPtsPerFrame = 90000 / 25;

// public:
//{{{
cRender::cRender (bool queued, const string& name, uint8_t streamTypeId, uint16_t decoderMask, size_t frameMapSize)
    : mFrameMapSize(frameMapSize), mQueued(queued),
      mName(name), mStreamTypeId(streamTypeId), mDecoderMask(decoderMask), mMiniLog ("log") {

  if (queued)
    thread ([=](){ startQueueThread(); }).detach();
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

// frames, freeframes
//{{{
cFrame* cRender::getFramePts (int64_t pts) {

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
  cFrame* youngestdFrame = (*it).second;
  mFrames.erase (it);

  return youngestdFrame;
  }
//}}}
//{{{
void cRender::addFreeFrame (cFrame* frame) {
// assumes mutex locked

  mFreeFrames.push_back (frame);
  }
//}}}
//{{{
void cRender::trimFramesBeforePts (int64_t pts) {
//trim frames before pts

  // anything to trim
  if (mFrames.empty())
    return;

  unique_lock<shared_mutex> lock(mSharedMutex);

  auto it = mFrames.begin();
  while ((it != mFrames.end()) && ((*it).first < pts)) {
    // remove frames before pts
    addFreeFrame ((*it).second);
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

/// queue
//{{{
int cRender::getQueueSize() const {
  return (int)mQueue.size_approx();
  }
//}}}
//{{{
float cRender::getQueueFrac() const {
  return (float)mQueue.size_approx() / mQueue.max_capacity();
  }
//}}}

// process
//{{{
bool cRender::processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {

  (void)skip;
  //log ("pes", fmt::format ("pts:{} size:{}", getFullPtsString (pts), pesSize));
  //logValue (pts, (float)pesSize);

  if (isQueued()) {
    mQueue.enqueue (new cDecoderQueueItem (mDecoder, pes, pesSize, pts, dts, mGetFrameCallback, mAddFrameCallback));
    return true;
    }
  else {
    mDecoder->decode (pes, pesSize, pts, dts, mGetFrameCallback, mAddFrameCallback);
    return false;
    }
  }
//}}}

// private:
//{{{
void cRender::startQueueThread() {

  cLog::setThreadName ("Q");

  mQueueRunning = true;

  while (!mQueueExit) {
    cDecoderQueueItem* queueItem;
    if (mQueue.wait_dequeue_timed (queueItem, 40000)) {
      queueItem->mDecoder->decode (queueItem->mPes, queueItem->mPesSize, queueItem->mPts, queueItem->mDts,
                                   queueItem->mGetFrameCallback, queueItem->mAddFrameCallback);
      delete queueItem;
      }
    }

  if (mQueue.size_approx())
    cLog::log (LOGINFO, fmt::format ("startQueueThread - should empty queue {}", mQueue.size_approx()));

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

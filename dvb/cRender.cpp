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
constexpr size_t kAudioFrameMapSize = 16;
constexpr int64_t kPtsPerFrame = 90000 / 25;

// public:
//{{{
cRender::cRender (bool queued, const string& name, uint8_t streamType, size_t frameMapSize)
    : mFrameMapSize(frameMapSize), mQueued(queued),
      mName(name), mStreamType(streamType), mMiniLog ("log") {

  if (queued)
    thread ([=](){ startQueueThread ("q" + name); }).detach();
  }
//}}}
//{{{
cRender::~cRender() {
  stopQueueThread();
  }
//}}}

//{{{
float cRender::getValue (int64_t pts) {

  unique_lock<shared_mutex> lock (mSharedMutex);

  auto it = mValuesMap.find (pts / kPtsPerFrame);
  return it == mValuesMap.end() ? 0.f : it->second;
  }
//}}}

//{{{
cFrame* cRender::getFreeFrame() {

  if (mFreeFrames.empty())
    return nullptr;

  else {
    unique_lock<shared_mutex> lock (mSharedMutex);

    cFrame* frame = mFreeFrames.front();
    mFreeFrames.pop_front();
    return frame;
    }
  }
//}}}
//{{{
cFrame* cRender::getYoungestFrame() {

  cFrame* youngestFrame;
  {
  unique_lock<shared_mutex> lock (mSharedMutex);

  // reuse youngest
  auto it = mFrames.begin();
  youngestFrame = (*it).second;
  mFrames.erase (it);
  }

  youngestFrame->releaseResources();
  return youngestFrame;
  }
//}}}
//{{{
cFrame* cRender::getFrameFromPts (int64_t pts) {

  //unique_lock<shared_mutex> lock (mSharedMutex);

  auto it = mFrames.find (pts);
  return (it == mFrames.end()) ? nullptr : it->second;
  }
//}}}
//{{{
cFrame* cRender::getNearestFrameFromPts (int64_t pts) {
// return nearest frame at or after pts

  unique_lock<shared_mutex> lock (mSharedMutex);

  auto it = mFrames.begin();
  while (it != mFrames.end()) {
    if (((*it).first - pts) >= 0)
      return (*it).second;
    ++it;
    }

  return nullptr;
  }
//}}}

//{{{
void cRender::trimFramesBeforePts (int64_t pts) {
// remove frames before pts, release any temp resources

  if (mFrames.empty())
    return;
  else {
    // locked
    unique_lock<shared_mutex> lock (mSharedMutex);

    auto it = mFrames.begin();
    while ((it != mFrames.end()) && ((*it).first < pts)) {
      it->second->releaseResources();
      mFreeFrames.push_back (it->second);
      it = mFrames.erase (it);
      }
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

// process
//{{{
bool cRender::processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {
  (void)skip;

  if (isQueued()) {
    mDecodeQueue.enqueue (new cDecodeQueueItem (mDecoder, pid, pes, pesSize, pts, dts, mAllocFrameCallback, mAddFrameCallback));
    return true;
    }
  else {
    mDecoder->decode (pid, pes, pesSize, pts, dts, mAllocFrameCallback, mAddFrameCallback);
    return false;
    }
  }
//}}}

// protected
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

// private:
//{{{
void cRender::startQueueThread (const string& name) {

  cLog::setThreadName (name);

  mQueueRunning = true;

  while (!mQueueExit) {
    cDecodeQueueItem* queueItem;
    if (mDecodeQueue.wait_dequeue_timed (queueItem, 40000)) {
      queueItem->mDecoder->decode (queueItem->mPid, queueItem->mPes, queueItem->mPesSize, queueItem->mPts, queueItem->mDts,
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

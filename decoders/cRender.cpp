 // cRender.cpp - dvb stream render base class - mFrameMap indexed by pts/ptsDuration
//{{{  includes
#include "cRender.h"

#include <cstdint>
#include <string>
#include <map>
#include <thread>

#include "../common/utils.h"
#include "../common/cLog.h"

#include "../decoders/cDecoder.h"

using namespace std;
using namespace utils;
//}}}
constexpr bool kLoadDebug = false;
constexpr bool kAllocFarthest = true;
constexpr bool kAllocAddFrameDebug = false;

//{{{
cRender::cRender (bool queued, const string& threadName,
                  uint8_t streamType, uint16_t pid,
                  int64_t ptsDuration, size_t maxFrames,
                  function <cFrame* (int64_t pts)> allocFrameCallback,
                  function <void (cFrame* frame)> addFrameCallback) :
    mQueued(queued), mThreadName(threadName),
    mStreamType(streamType), mPid(pid),
    mMaxFrames(maxFrames),
    mAllocFrameCallback(allocFrameCallback),
    mAddFrameCallback(addFrameCallback),
    mPtsDuration(ptsDuration) {

  if (queued)
    thread ([=](){ startQueueThread (threadName + "q"); }).detach();
  }
//}}}
//{{{
cRender::~cRender() {

  stopQueueThread();
  clearFrames();
  }
//}}}

//{{{
bool cRender::isFrameAtPts (int64_t pts) {
// return true if pts in mFramesMap

  // lock
  unique_lock<shared_mutex> lock (mSharedMutex);
  return isFrameAtPtsAlreadyLocked (pts);
  }
//}}}
//{{{
cFrame* cRender::getFrameAtPts (int64_t pts) {
// find frame containing pts
// - !!! could speed up with map.upper_bound !!!

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto it = mFramesMap.begin(); it != mFramesMap.end(); ++it)
    if (it->second->contains (pts))
      return it->second;

  return nullptr;
  }
//}}}
//{{{
cFrame* cRender::getFrameAtOrAfterPts (int64_t pts) {
// return frame at or firstFrame after pts
// - useful for skipping gaps

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto it = mFramesMap.begin(); it != mFramesMap.end(); ++it)
    if (it->first >= pts)
      return it->second;

  return nullptr;
  }
//}}}

//{{{
void cRender::setPts (int64_t pts, int64_t ptsDuration) {

  mPts = pts;
  mPtsDuration = ptsDuration;
  mPtsEnd = pts + ptsDuration;

  if (pts != -1) {
    if (mFirstPts == -1)
      mFirstPts = pts;
    if (pts > mLastPts)
      mLastPts = pts;
    }
  }
//}}}

//{{{
cFrame* cRender::allocFrame (int64_t pts) {

  cFrame* frame = nullptr;

  { // lock
  unique_lock<shared_mutex> lock (mSharedMutex);

  if (isFrameAtPtsAlreadyLocked (pts)) {
    // don't alloc
    cLog::log (LOGINFO, fmt::format ("- allocFrame {} already decoded", getPtsString (pts)));
    return nullptr;
    }

  // if pts is farther from front than back of mFramesMap, alloc front
  int64_t frontDist = pts - mFramesMap.begin()->second->getPts();
  int64_t backDist = mFramesMap.rbegin()->second->getPts() - pts;
  auto it = frontDist > backDist ? mFramesMap.begin() : --mFramesMap.end();
  frame = it->second;
  mFramesMap.erase (it);
  }

  if (kAllocAddFrameDebug)
    cLog::log (LOGINFO, fmt::format ("- allocFrame {}", getPtsString (frame->getPts())));

  frame->releaseResources();
  return frame;
  }
//}}}
//{{{
void cRender::addFrame (cFrame* frame) {

  if (kAllocAddFrameDebug)
    cLog::log (LOGINFO, fmt::format ("- addFrame {}", getPtsString (frame->getPts())));

  unique_lock<shared_mutex> lock (mSharedMutex);
  if (mFramesMap.find (frame->getPts()) != mFramesMap.end())
    cLog::log (LOGERROR, fmt::format ("- addFrame - trying to add duplicate pts:{}",
                                      getPtsString(frame->getPts()) ));
  else
    mFramesMap.emplace (frame->getPts(), frame);
  }
//}}}

// process
//{{{
bool cRender::throttle (int64_t pts) {

  // lock
  unique_lock<shared_mutex> lock (mSharedMutex);

  if (mFramesMap.size() < mMaxFrames)
    return false;

  size_t numFramesAfterPts = 0;
  auto it = mFramesMap.begin();
  while (it != mFramesMap.end()) {
    if (pts < it->second->getPts())
      numFramesAfterPts++;
    ++it;
    }

  return numFramesAfterPts >= mMaxFrames/2;
  }
//}}}
//{{{
void cRender::decodePes (uint8_t* pes, uint32_t pesSize, int64_t pts, char frameType) {

  if (isQueued())
    mDecodeQueue.enqueue (new cDecodeQueueItem (mDecoder, pes, pesSize, pts, frameType,
                                                mAllocFrameCallback, mAddFrameCallback));
  else
    mDecoder->decode (pes, pesSize, pts, frameType, mAllocFrameCallback, mAddFrameCallback);
  }
//}}}

//{{{
string cRender::getInfoString() const {
  return fmt::format ("frames:{:2d}:{:d} pts:{} dur:{}",
                      mFramesMap.size(), getQueueSize(), getPtsString (mPts), mPtsDuration);
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
void cRender::clearFrames() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFramesMap)
    delete (frame.second);

  mFramesMap.clear();
  }
//}}}

//{{{
bool cRender::isFrameAtPtsAlreadyLocked (int64_t pts) {
// return true if pts in mFramesMap

  for (auto it = mFramesMap.begin(); it != mFramesMap.end(); ++it)
    if (it->second->contains (pts))
      return true;

  return false;
  }
//}}}

//{{{
void cRender::startQueueThread (const string& name) {

  cLog::setThreadName (name);

  mQueueRunning = true;

  while (!mQueueExit) {
    cDecodeQueueItem* queueItem;
    if (mDecodeQueue.wait_dequeue_timed (queueItem, 40000)) {
      queueItem->mDecoder->decode (queueItem->mPes, queueItem->mPesSize,
                                   queueItem->mPts, queueItem->mFrameType,
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

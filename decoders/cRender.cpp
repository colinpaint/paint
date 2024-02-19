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
constexpr bool kAllocAddFrameDebug = false;

//{{{
cRender::cRender (bool queued, const string& threadName,
                  uint8_t streamType, uint16_t pid,
                  int64_t ptsDuration, size_t maxFrames, size_t preLoadFrames,
                  function <cFrame* (int64_t pts, bool front)> allocFrameCallback,
                  function <void (cFrame* frame)> addFrameCallback) :
    mQueued(queued), mThreadName(threadName),
    mStreamType(streamType), mPid(pid),
    mMaxFrames(maxFrames), mPreLoadFrames(preLoadFrames),
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
cFrame* cRender::getFrameAtPts (int64_t pts) {
// find pts frame

  unique_lock<shared_mutex> lock (mSharedMutex);

  auto it = mFramesMap.find (pts / mPtsDuration);
  return (it == mFramesMap.end()) ? nullptr : it->second;
  }
//}}}
//{{{
cFrame* cRender::getFrameAtOrAfterPts (int64_t pts) {
// search for first frame, on or after pts

  unique_lock<shared_mutex> lock (mSharedMutex);

  auto it = mFramesMap.begin();
  while (it != mFramesMap.end()) {
    int64_t diff = it->first - (pts / mPtsDuration);
    if (diff >= 0)
      return it->second;
    ++it;
    }

  return nullptr;
  }
//}}}

//{{{
void cRender::setPts (int64_t pts, int64_t ptsDuration) {

  mPts = pts;
  mPtsDuration = ptsDuration;

  if (pts != -1) {
    if (mFirstPts == -1)
      mFirstPts = pts;

    if (pts > mLastPts)
      mLastPts = pts;
    }
  }
//}}}

//{{{
cFrame* cRender::allocFrame (int64_t pts, bool front) {

  cFrame* frame = nullptr;

  { // lock
  unique_lock<shared_mutex> lock (mSharedMutex);

  if (findLocked (pts)) {
    // don't alloc
    if (kAllocAddFrameDebug)
      cLog::log (LOGINFO, fmt::format ("- allocFrame {} already decoded", getFullPtsString (pts)));
    return nullptr;
    }

  auto it = front ? mFramesMap.begin() : --mFramesMap.end();
  frame = it->second;
  mFramesMap.erase (it);
  }

  if (kAllocAddFrameDebug)
    cLog::log (LOGINFO, fmt::format ("- allocFrame {} from {}",
                                     getFullPtsString (frame->getPts()), front?"front":"back"));

  frame->releaseResources();
  return frame;
  }
//}}}
//{{{
void cRender::addFrame (cFrame* frame) {

  if (kAllocAddFrameDebug)
    cLog::log (LOGINFO, fmt::format ("- addFrame {}", getFullPtsString (frame->getPts())));

  unique_lock<shared_mutex> lock (mSharedMutex);
  if (mFramesMap.find (frame->getPts() / frame->getPtsDuration()) != mFramesMap.end())
    cLog::log (LOGERROR, fmt::format ("addFrame duplicate"));
  else
    mFramesMap.emplace (frame->getPts() / frame->getPtsDuration(), frame);
  }
//}}}
//{{{
void cRender::clearFrames() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFramesMap)
    delete (frame.second);

  mFramesMap.clear();
  }
//}}}

// process
//{{{
int64_t cRender::load (int64_t pts, bool& allocFront) {
// return first unloaded frame in preLoad order
// - 0, 1..maxPreLoadFrames, -maxPreLoadFrames..-1

  // load pts
  allocFront = true;
  int index = 0;
  if (!find (pts)) {
    if (kLoadDebug)
      cLog::log (LOGINFO, fmt::format ("load index:{} pts:{}", index, getFullPtsString (pts)));
    return pts;
    }

  // preload after pts
  for (index = 1; index <= (int)mPreLoadFrames; index++) {
    int64_t loadPts = pts + (index * mPtsDuration);
    if (!find (loadPts)) {
      if (kLoadDebug)
        cLog::log (LOGINFO, fmt::format ("load index:{} pts:{}", index, getFullPtsString (loadPts)));
      return loadPts;
      }
    }

  // preload before pts in forward order to help decoder
  for (index = -(int)mPreLoadFrames; index <= -1; index++) {
    int64_t loadPts = pts + (index * mPtsDuration);
    if (!find (loadPts)) {
      allocFront = false;
      if (kLoadDebug)
        cLog::log (LOGINFO, fmt::format ("load index:{} pts:{}", index, getFullPtsString (loadPts)));
      return loadPts;
      }
    }

  // nothing to load
  return -1;
  }
//}}}
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
void cRender::decodePes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool allocFront) {

  if (isQueued())
    mDecodeQueue.enqueue (new cDecodeQueueItem (mDecoder, pes, pesSize, pts, dts,
                                                allocFront, mAllocFrameCallback, mAddFrameCallback));
  else
    mDecoder->decode (pes, pesSize, pts, dts,
                      allocFront, mAllocFrameCallback, mAddFrameCallback);
  }
//}}}

//{{{
string cRender::getInfoString() const {
  return fmt::format ("frames:{:2d}:{:d} pts:{} dur:{}",
                      mFramesMap.size(), getQueueSize(), getFullPtsString (mPts), mPtsDuration);
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
bool cRender::find (int64_t pts) {
// return true if pts in mFramesMap

  // lock
  unique_lock<shared_mutex> lock (mSharedMutex);
  return findLocked (pts);
  }
//}}}
//{{{
bool cRender::findLocked (int64_t pts) {
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
                                   queueItem->mPts, queueItem->mDts, queueItem->mAllocFront,
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

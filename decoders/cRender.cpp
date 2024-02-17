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
constexpr size_t kMaxLogSize = 64;

//{{{
cRender::cRender (bool queued, const string& threadName,
                  uint8_t streamType, uint16_t pid,
                  int64_t ptsDuration, size_t maxFrames,
                  function <cFrame* (bool front)> allocFrameCallback,
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
void cRender::addFrame (cFrame* frame) {

  //cLog::log (LOGINFO, fmt::format ("addFrame {} {}",
  //                                 getFullPtsString (frame->getPts()),
  //                                 getFullPtsString ((frame->getPts() / frame->getPtsDuration()) * frame->getPtsDuration())));
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFramesMap.emplace (frame->getPts() / frame->getPtsDuration(), frame);
  }
//}}}
//{{{
cFrame* cRender::removeFrame (bool front) {

  cFrame* frame = nullptr;

  { // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  auto it = front ? mFramesMap.begin() : --mFramesMap.end();
  frame = it->second;
  mFramesMap.erase (it);
  }

  //cLog::log (LOGINFO, fmt::format ("removeFrame {} {}",
  //                                 front ? "front":"back", getPtsString (frame->getPts())));
  frame->releaseResources();
  return frame;
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
bool cRender::found (int64_t pts) {
// return true if pts i nmFramesMap range

  { // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto pair : mFramesMap)
    if ((pts >= pair.second->getPts()) && (pts < (pair.second->getPts() + mPtsDuration)))
      return true;

  return false;
  }
  }
//}}}
//{{{
bool cRender::throttle (int64_t pts) {
// return true if fileRead should throttle
// - if mFramesMap not reached max, and frames beforePts < mMaxFrames/2

  if (mFramesMap.size() < mMaxFrames) // no throttle
    return false;

  { // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  size_t numFramesBeforePts = 0;
  auto it = mFramesMap.begin();
  while ((it != mFramesMap.end()) && (it->first < (pts / mPtsDuration))) {
    if (++numFramesBeforePts >= mMaxFrames/2) // if mFramesMap at least centred about pts, no throttle
      return false;
    ++it;
    }

  // not centred about pts, throttle
  return true;
  }
  }
//}}}
//{{{
void cRender::decodePes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool allocFront) {

  if (isQueued())
    mDecodeQueue.enqueue (new cDecodeQueueItem (mDecoder, pes, pesSize, pts, dts, allocFront,
                                                mAllocFrameCallback, mAddFrameCallback));
  else
    mDecoder->decode (pes, pesSize, pts, dts, allocFront, mAllocFrameCallback, mAddFrameCallback);
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

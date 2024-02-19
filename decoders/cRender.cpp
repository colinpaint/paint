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
constexpr bool kLoadDebug = true;
constexpr bool kAddFrameDebug = false;
constexpr bool kRemoveFrameDebug = true;

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
void cRender::addFrame (cFrame* frame) {

  if (kAddFrameDebug)
    cLog::log (LOGINFO, fmt::format ("addFrame {} {}",
                                     getFullPtsString (frame->getPts()),
                                     getFullPtsString ((frame->getPts() / frame->getPtsDuration()) * frame->getPtsDuration())));
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFramesMap.emplace (frame->getPts() / frame->getPtsDuration(), frame);
  }
//}}}
//{{{
cFrame* cRender::removeFrame (int64_t pts, bool front) {

  cFrame* frame = nullptr;

  // if frame find, return it, else reuse front/back
  //auto it = mFramesMap.find (pts / mPtsDuration);
  //if (it == mFramesMap.end())
  //  it = front ? mFramesMap.begin() : --mFramesMap.end();

  { // locked
  unique_lock<shared_mutex> lock (mSharedMutex);
  auto it = front ? mFramesMap.begin() : --mFramesMap.end();
  frame = it->second;
  mFramesMap.erase (it);
  }

  if (kRemoveFrameDebug)
    cLog::log (LOGINFO, fmt::format ("cRender::removeFrame pts:{} frame:{} {}",
                                     getCompletePtsString (pts),
                                     getCompletePtsString (frame->getPts()),
                                     front ?"front":"back"));

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
int64_t cRender::load (int64_t pts, int& index) {

  // load pts
  index = 0;
  if (!found (pts)) {
    if (kLoadDebug)
      cLog::log (LOGINFO, fmt::format ("load index:{} pts:{}", index, getCompletePtsString (pts)));
    return pts;
    }

  // preload after pts
  for (index = 1; index <= (int)mPreLoadFrames; index++) {
    int64_t loadPts = pts + (index * mPtsDuration);
    if (!found (loadPts)) {
      if (kLoadDebug)
        cLog::log (LOGINFO, fmt::format ("load index:{} pts:{}", index, getCompletePtsString (loadPts)));
      return loadPts;
      }
    }

  // preload before pts in forward order to help decoder
  for (index = -(int)mPreLoadFrames; index <= -1; index++) {
    int64_t loadPts = pts + (index * mPtsDuration);
    if (!found (loadPts)) {
      if (kLoadDebug)
        cLog::log (LOGINFO, fmt::format ("load index:{} pts:{}", index, getCompletePtsString (loadPts)));
      return loadPts;
      }
    }

  // nothing to load
  return -1;
  }
//}}}
//{{{
bool cRender::found (int64_t pts) {
// return true if pts in mFramesMap

  // locked
  unique_lock<shared_mutex> lock (mSharedMutex);
  return mFramesMap.find (pts / mPtsDuration) != mFramesMap.end();
  }
//}}}
//{{{
bool cRender::throttle (int64_t pts) {

  // locked
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

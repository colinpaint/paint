// cRender.cpp - dvb stream render base class
// - mFrameMap indexed by pts/ptsDuration
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
//}}}
constexpr size_t kMaxLogSize = 64;

//{{{
cRender::cRender (bool queued, const string& name, const string& threadName,
                  uint8_t streamType, uint16_t pid,
                  int64_t ptsDuration, size_t maxFrames,
                  function <cFrame* ()> getFrameCallback,
                  function <void (cFrame* frame)> addFrameCallback) :
    mQueued(queued), mName(name), mThreadName(threadName),
    mStreamType(streamType), mPid(pid),
    mMaxFrames(maxFrames),
    mGetFrameCallback(getFrameCallback),
    mAddFrameCallback(addFrameCallback),
    mMiniLog ("log"), mMaxLogSize(kMaxLogSize),
    mPtsDuration(ptsDuration) {

  if (queued)
    thread ([=](){ startQueueThread (threadName + "q"); }).detach();
  }
//}}}
//{{{
cRender::~cRender() {

  stopQueueThread();

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFramesMap)
    delete (frame.second);
  mFramesMap.clear();
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
cFrame* cRender::reuseBestFrame() {

  cFrame* frame;

  { // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  // reuse youngestfor now, could be different if seeking
  auto it = mFramesMap.begin();
  frame = it->second;
  mFramesMap.erase (it);
  }

  //cLog::log (LOGINFO, fmt::format ("reuseBestFrame:{}", utils::getFullPtsString (frame->getPts())));
  frame->releaseResources();
  return frame;
  }
//}}}
//{{{
void cRender::addFrame (cFrame* frame) {

  //cLog::log (LOGINFO, fmt::format ("addFrame {} {}",
  //                                 utils::getFullPtsString (frame->getPts()),
  //                                 utils::getFullPtsString ((frame->getPts() / frame->getPtsDuration()) * frame->getPtsDuration())));
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFramesMap.emplace (frame->getPts() / frame->getPtsDuration(), frame);
  }
//}}}

// process
//{{{
string cRender::getInfoString() const {
  return fmt::format ("frames:{:2d}:{:d} pts:{} dur:{}",
                      mFramesMap.size(), getQueueSize(), utils::getFullPtsString (mPts), mPtsDuration);
  }
//}}}
//{{{
bool cRender::processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {
  (void)skip;

  if (isQueued()) {
    mDecodeQueue.enqueue (new cDecodeQueueItem (mDecoder, pid, pes, pesSize, pts, dts, mGetFrameCallback, mAddFrameCallback));
    return true;
    }
  else {
    mDecoder->decode (pid, pes, pesSize, pts, dts, mGetFrameCallback, mAddFrameCallback);
    return false;
    }
  }
//}}}

// log
void cRender::toggleLog() { mMiniLog.toggleEnable(); }
void cRender::header() { mMiniLog.setHeader (fmt::format ("header")); }
void cRender::log (const string& tag, const string& text) { mMiniLog.log (tag, text); }

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

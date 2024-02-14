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
//}}}
constexpr size_t kMaxLogSize = 64;

//{{{
cRender::cRender (bool queued, const string& name, const string& threadName, iOptions* options,
                  uint8_t streamType, uint16_t pid,
                  int64_t ptsDuration, size_t maxFrames,
                  function <cFrame* ()> getFrameCallback,
                  function <void (cFrame* frame)> addFrameCallback) :
    mOptions(options),
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
void cRender::setPts (int64_t pts, int64_t ptsDuration, int64_t streamPos) {

  mPts = pts;
  mPtsDuration = ptsDuration;

  if (pts != -1) {
    if (mFirstPts == -1)
      mFirstPts = pts;

    if (pts > mFirstPts)
      mStreamPosPerPts = streamPos / float(pts - mFirstPts);

    if (pts > mLastPts)
      mLastPts = pts;
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
bool cRender::decodePes (uint8_t* pes, uint32_t pesSize,
                         int64_t pts, int64_t dts, int64_t streamPos, bool skip) {

  if (isQueued()) {
    mDecodeQueue.enqueue (new cDecodeQueueItem (mDecoder, pes, pesSize,
                                                pts, dts, streamPos, skip,
                                                mGetFrameCallback, mAddFrameCallback));
    return true;
    }

  mDecoder->decode (pes, pesSize, pts, dts, streamPos, skip, mGetFrameCallback, mAddFrameCallback);
  return false;
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
      queueItem->mDecoder->decode (queueItem->mPes, queueItem->mPesSize,
                                   queueItem->mPts, queueItem->mDts, queueItem->mStreamPos, queueItem->mSkip,
                                   queueItem->mGetFrameCallback, queueItem->mAddFrameCallback);
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
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
//{{{  defines
#define AVRB16(p) ((*(p) << 8) | *(p+1))

#define SCALEBITS 10
#define ONE_HALF  (1 << (SCALEBITS - 1))
#define FIX(x)    ((int) ((x) * (1<<SCALEBITS) + 0.5))
//}}}
constexpr int64_t kPtsPerFrame = 90000 / 25;

// public:
//{{{
cRender::cRender (bool queued, const string& name, uint8_t streamType, uint16_t decoderMask)
    : mQueued(queued), mName(name), mStreamType(streamType), mDecoderMask(decoderMask), mMiniLog ("log") {

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

void cRender::toggleLog() { mMiniLog.toggleEnable(); }
//{{{
void cRender::logValue (int64_t pts, float value) {

  while (mValuesMap.size() >= mMapSize)
    mValuesMap.erase (mValuesMap.begin());

  mValuesMap.emplace (pts / kPtsPerFrame, value);

  if (pts > mLastPts)
    mLastPts = pts;
  }
//}}}
void cRender::log (const string& tag, const string& text) { mMiniLog.log (tag, text); }
void cRender::header() { mMiniLog.setHeader (fmt::format ("header")); }

/// queue
//{{{
void cRender::startQueueThread() {

  cLog::setThreadName ("Q");

  mQueueRunning = true;

  while (!mQueueExit) {
    cDecoderQueueItem* queueItem;
    if (mQueue.wait_dequeue_timed (queueItem, 40000)) {
      queueItem->mDecoder->decode (queueItem->mPes, queueItem->mPesSize, queueItem->mPts, queueItem->mDts,
                                   queueItem->mAddFrameCallback);
      delete queueItem;
      }
    }

  // !!! not sure this is empty the queue on exit !!!!

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
    mQueue.enqueue (new cDecoderQueueItem (mDecoder, pes, pesSize, pts, dts, mAddFrameCallback));
    return true;
    }
  else {
    mDecoder->decode (pes, pesSize, pts, dts, mAddFrameCallback);
    return false;
    }
  }
//}}}

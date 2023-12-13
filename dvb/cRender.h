// cRender.h - stream render base class
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <deque>
#include <functional>
#include <mutex>
#include <shared_mutex>

#include "../common/cMiniLog.h"
#include "../common/readerWriterQueue.h"

#include "cFrame.h"
class cDecoder;
#include "cDecoderQueueItem.h"
//}}}
constexpr int kPtsPerSecond = 90000;

class cRender {
public:
  cRender (bool queued, const std::string& name, uint8_t streamType, uint16_t pid,
           int64_t ptsDuration, bool live, size_t maxFrames);
  virtual ~cRender();

  bool isQueued() const { return mQueued; }
  int64_t getPts() const { return mPts; }
  int64_t getPtsDuration() const { return mPtsDuration; }
  uint16_t getPid() const { return mPid; }

  std::shared_mutex& getSharedMutex() { return mSharedMutex; }
  cMiniLog& getLog() { return mMiniLog; }
  float getValue (int64_t pts);

  int64_t getLastPts() const { return mLastPts; }
  size_t getNumValues() const { return mValuesMap.size(); }
  std::map<int64_t,cFrame*> getFramesMap() { return mFramesMap; }

  cFrame* getFrameAtPts (int64_t pts);
  cFrame* getFrameAtOrAfterPts (int64_t pts);

  void setAllocFrameCallback (std::function <cFrame* ()> getFrameCallback) { mAllocFrameCallback = getFrameCallback; }
  void setAddFrameCallback (std::function <void (cFrame* frame)> addFrameCallback) { mAddFrameCallback = addFrameCallback; }

  bool throttle (int64_t pts);
  cFrame* reuseBestFrame();
  virtual void addFrame (cFrame* frame);

  void toggleLog();
  void header();
  void log (const std::string& tag, const std::string& text);
  void logValue (int64_t pts, float value);

  virtual std::string getInfoString() const;
  virtual bool processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip);

protected:
  size_t getQueueSize() const;
  float getQueueFrac() const;

  std::shared_mutex mSharedMutex;
  cDecoder* mDecoder = nullptr;

  // frameMap
  std::map <int64_t, cFrame*> mFramesMap;

  int64_t mPts = 0;
  int64_t mPtsDuration = 0;

  // decode queue
  bool mQueueExit = false;
  bool mQueueRunning = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cDecodeQueueItem*> mDecodeQueue;

private:
  void startQueueThread (const std::string& name);
  void stopQueueThread();

  const bool mQueued = false;
  const std::string mName;
  const uint8_t mStreamType;
  const uint16_t mPid;

  const bool mLive;
  const size_t mMaxFrames;

  cMiniLog mMiniLog;
  size_t mMaxLogSize = 64;

  std::function <cFrame* ()> mAllocFrameCallback;
  std::function <void (cFrame* frame)> mAddFrameCallback;

  // plot
  std::map <int64_t,float> mValuesMap;
  int64_t mLastPts = 0;
  };

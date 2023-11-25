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
  cRender (bool queued, const std::string& name, uint8_t streamType, size_t frameMapSize, bool realTime);
  virtual ~cRender();

  bool isQueued() const { return mQueued; }
  float getRealTime() const { return mRealTime; }

  std::shared_mutex& getSharedMutex() { return mSharedMutex; }
  cMiniLog& getLog() { return mMiniLog; }
  float getValue (int64_t pts);

  int64_t getLastPts() const { return mLastPts; }
  size_t getNumValues() const { return mValuesMap.size(); }
  size_t getFrameMapSize() const { return mFrameMapSize; }
  std::map<int64_t,cFrame*> getFramesMap() { return mFramesMap; }
  std::deque<cFrame*> getFreeFrames() { return mFreeFrames; }

  cFrame* getFreeFrame();
  cFrame* getYoungestFrame();
  cFrame* getFrameFromPts (int64_t pts, int64_t duration, bool locked);
  cFrame* getNearestFrameFromPts (int64_t pts);

  void setAllocFrameCallback (std::function <cFrame* ()> getFrameCallback) { mAllocFrameCallback = getFrameCallback; }
  void setAddFrameCallback (std::function <void (cFrame* frame)> addFrameCallback) { mAddFrameCallback = addFrameCallback; }
  void setMapSize (size_t size) { mMapSize = size; }

  void toggleLog();
  void header();
  void log (const std::string& tag, const std::string& text);
  void logValue (int64_t pts, float value);

  virtual std::string getInfoString() const = 0;
  virtual bool processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip);

  virtual void trimPts (int64_t pts);
  virtual void trimBeforePts (int64_t pts);

protected:
  size_t getQueueSize() const;
  float getQueueFrac() const;

  std::shared_mutex mSharedMutex;
  cDecoder* mDecoder = nullptr;

  // frameMap
  size_t mFrameMapSize;
  std::map <int64_t, cFrame*> mFramesMap;
  std::deque <cFrame*> mFreeFrames;

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
  const bool mRealTime;

  std::function <cFrame* ()> mAllocFrameCallback;
  std::function <void (cFrame* frame)> mAddFrameCallback;

  cMiniLog mMiniLog;

  // plot
  std::map <int64_t,float> mValuesMap;
  int64_t mLastPts = 0;
  size_t mMapSize = 64;
  };

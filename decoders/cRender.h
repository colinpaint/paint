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

#include "../common/basicTypes.h"
#include "../common/iOptions.h"
#include "../common/readerWriterQueue.h"

#include "../decoders/cFrame.h"
class cDecoder;
#include "../decoders/cDecoderQueueItem.h"
//}}}

class cRender {
public:
  cRender (bool queued, const std::string& threadName,
           uint8_t streamType, uint16_t pid,
           int64_t ptsDuration, size_t maxFrames,
           std::function <cFrame* (int64_t pts)> allocFrameCallback,
           std::function <void (cFrame* frame)> addFrameCallback);
  virtual ~cRender();

  bool isQueued() const { return mQueued; }
  uint16_t getPid() const { return mPid; }

  int64_t getPts() const { return mPts; }
  int64_t getPtsDuration() const { return mPtsDuration; }
  int64_t getPtsEnd() const { return mPts; }
  int64_t getFirstPts() const { return mFirstPts; }
  int64_t getLastPts() const { return mLastPts; }

  std::shared_mutex& getSharedMutex() { return mSharedMutex; }
  std::map<int64_t,cFrame*> getFramesMap() { return mFramesMap; }
  bool hasMaxFrames() const { return mFramesMap.size() >= mMaxFrames; }

  void setPts (int64_t pts, int64_t ptsDuration);

  // finds
  bool isFrameAtPts (int64_t pts);
  cFrame* getFrameAtPts (int64_t pts);
  cFrame* getFrameAtOrAfterPts (int64_t pts);
  cFrame* allocFrame (int64_t pts);
  void addFrame (cFrame* frame);

  bool throttle (int64_t pts);
  void decodePes (uint8_t* pes, uint32_t pesSize, int64_t pts, char frameType);

  virtual std::string getInfoString() const;
  virtual bool throttle() { return false; }
  virtual void togglePlay() {}

protected:
  size_t getQueueSize() const;
  float getQueueFrac() const;

  void clearFrames();

  bool isFrameAtPtsAlreadyLocked (int64_t pts);

  // vars
  std::shared_mutex mSharedMutex;
  cDecoder* mDecoder = nullptr;

  // frameMap
  std::map <int64_t, cFrame*> mFramesMap;

  // decode queue
  bool mQueueExit = false;
  bool mQueueRunning = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cDecodeQueueItem*> mDecodeQueue;

private:
  void startQueueThread (const std::string& name);
  void stopQueueThread();

  // vars
  const bool mQueued;
  const std::string mThreadName;
  const uint8_t mStreamType;
  const uint16_t mPid;
  const size_t mMaxFrames;
  const std::function <cFrame* (int64_t pts)> mAllocFrameCallback;
  const std::function <void (cFrame* frame)> mAddFrameCallback;

  int64_t mPts = -1;
  int64_t mPtsDuration = 0;
  int64_t mPtsEnd = -1;
  int64_t mFirstPts = -1;
  int64_t mLastPts = -1;
  };

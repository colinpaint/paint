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
#include "../common/cMiniLog.h"
#include "../common/readerWriterQueue.h"

#include "../decoders/cFrame.h"
class cDecoder;
#include "../decoders/cDecoderQueueItem.h"
//}}}

class cRender {
public:
  //{{{
  class cOptions {
  public:
    virtual ~cOptions() = default;

    bool mIsLive = false;
    };
  //}}}

  cRender (bool queued, const std::string& name, const std::string& threadName, iOptions* options,
           uint8_t streamType, uint16_t pid,
           int64_t ptsDuration, size_t maxFrames,
           std::function <cFrame* ()> getFrameCallback,
           std::function <void (cFrame* frame)> addFrameCallback);
  virtual ~cRender();

  bool isQueued() const { return mQueued; }
  uint16_t getPid() const { return mPid; }
  cMiniLog& getLog() { return mMiniLog; }

  int64_t getPts() const { return mPts; }
  int64_t getPtsFromStart() const { return mPts - mFirstPts; }
  int64_t getPtsDuration() const { return mPtsDuration; }
  float getStreamPosPerPts() const { return mStreamPosPerPts; }

  std::shared_mutex& getSharedMutex() { return mSharedMutex; }
  std::map<int64_t,cFrame*> getFramesMap() { return mFramesMap; }
  bool hasMaxFrames() const { return mFramesMap.size() >= mMaxFrames; }

  void setPts (int64_t pts, int64_t ptsDuration, int64_t streamPos);

  cFrame* getFrameAtPts (int64_t pts);
  cFrame* getFrameAtOrAfterPts (int64_t pts);
  cFrame* reuseBestFrame();
  void addFrame (cFrame* frame);

  bool decodePes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, int64_t streamPos = 0);
  virtual bool throttle (int64_t pts);

  virtual std::string getInfoString() const;
  virtual bool throttle() { return false; }
  virtual void togglePlay() {}

  void toggleLog();
  void header();
  void log (const std::string& tag, const std::string& text);

protected:
  size_t getQueueSize() const;
  float getQueueFrac() const;

  // vars
  iOptions* mOptions;

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
  const std::string mName;
  const std::string mThreadName;
  const uint8_t mStreamType;
  const uint16_t mPid;
  const size_t mMaxFrames;
  const std::function <cFrame* ()> mGetFrameCallback;
  const std::function <void (cFrame* frame)> mAddFrameCallback;

  cMiniLog mMiniLog;
  const size_t mMaxLogSize;

  int64_t mPts = 0;
  int64_t mPtsDuration = 0;
  int64_t mFirstPts = -1;
  int64_t mLastPts = -1;
  float mStreamPosPerPts = 1.f;
  };

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

class cRender;
//}}}
constexpr int kPtsPerSecond = 90000;

//{{{
class cFrame {
public:
  cFrame() = default;
  virtual ~cFrame() = default;

  virtual void releaseResources() = 0;

  // vars
  int64_t mPts;
  int64_t mPtsDuration;
  uint32_t mPesSize;
  };
//}}}
//{{{
class cDecoder {
public:
  cDecoder() = default;
  virtual ~cDecoder() = default;

  virtual std::string getInfoString() const = 0;

  virtual int64_t decode (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          std::function<cFrame*()> allocFrameCallback,
                          std::function<void (cFrame* frame)> addFrameCallback) = 0;
  };
//}}}
//{{{
class cDecodeQueueItem {
public:
  //{{{
  cDecodeQueueItem (cDecoder* decoder,
                    uint16_t pid, uint8_t* pes, int pesSize, int64_t pts, int64_t dts,
                    std::function<cFrame*()> allocFrameCallback,
                    std::function<void (cFrame* frame)> addFrameCallback)
      : mDecoder(decoder),
        mPid(pid), mPes(pes), mPesSize(pesSize), mPts(pts), mDts(dts),
        mAllocFrameCallback(allocFrameCallback), mAddFrameCallback(addFrameCallback) {}
  // we gain ownership of malloc'd pes buffer
  //}}}
  ~cDecodeQueueItem() {
    // release malloc'd pes buffer
    free (mPes);
    }

  cDecoder* mDecoder;

  uint16_t mPid;
  uint8_t* mPes;
  const int mPesSize;

  const int64_t mPts;
  const int64_t mDts;

  const std::function<cFrame*()> mAllocFrameCallback;
  const std::function<void (cFrame* frame)> mAddFrameCallback;
  };
//}}}

class cRender {
public:
  enum eDecoder { eFFmpeg, eMfxSystem, eMfxVideo };

  cRender (bool queued, const std::string& name, uint8_t streamType, uint16_t decoderMask, size_t frameMapSize);
  virtual ~cRender();

  bool isQueued() const { return mQueued; }

  std::shared_mutex& getSharedMutex() { return mSharedMutex; }
  cMiniLog& getLog() { return mMiniLog; }
  float getValue (int64_t pts);

  int64_t getLastPts() const { return mLastPts; }
  size_t getNumValues() const { return mValuesMap.size(); }
  size_t getFrameMapSize() const { return mFrameMapSize; }
  std::map<int64_t,cFrame*> getFrames() { return mFrames; }
  std::deque<cFrame*> getFreeFrames() { return mFreeFrames; }

  cFrame* getFreeFrame();
  cFrame* getYoungestFrame();
  cFrame* getFrameFromPts (int64_t pts);
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
  virtual void trimFramesBeforePts (int64_t pts);

protected:
  size_t getQueueSize() const;
  float getQueueFrac() const;

  std::shared_mutex mSharedMutex;
  cDecoder* mDecoder = nullptr;

  // frameMap
  size_t mFrameMapSize;
  std::map <int64_t, cFrame*> mFrames;
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
  const uint16_t mDecoderMask;

  std::function <cFrame* ()> mAllocFrameCallback;
  std::function <void (cFrame* frame)> mAddFrameCallback;

  cMiniLog mMiniLog;

  // plot
  std::map <int64_t,float> mValuesMap;
  int64_t mLastPts = 0;
  size_t mMapSize = 64;
  };

// cRender.h - stream render base class
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <functional>
#include <mutex>
#include <shared_mutex>

#include "../utils/cMiniLog.h"
#include "../utils/readerWriterQueue.h"

class cRender;
//}}}
constexpr int kPtsPerSecond = 90000;

//{{{
class cFrame {
public:
  cFrame (int64_t pts, int64_t ptsDuration) : mPts(pts), mPtsDuration(ptsDuration) {}
  virtual ~cFrame() = default;

  int64_t getPts() const { return mPts; }
  int64_t getPtsDuration() const { return mPtsDuration; }

protected:
  int64_t mPts;
  int64_t mPtsDuration;
  };
//}}}
//{{{
class cDecoder {
public:
  cDecoder() = default;
  virtual ~cDecoder() = default;

  virtual std::string getName() const = 0;

  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          std::function<void (cFrame* frame)> addFrameCallback) = 0;
  };
//}}}
//{{{
class cDecoderQueueItem {
public:
  //{{{
  cDecoderQueueItem (cDecoder* decoder,
                     uint8_t* pes, int pesSize, int64_t pts, int64_t dts,
                     std::function<void (cFrame* frame)> addFrameCallback)
      : mDecoder(decoder),
        mPes(pes), mPesSize(pesSize), mPts(pts), mDts(dts),
        mAddFrameCallback(addFrameCallback) {}
  // we gain ownership of malloc'd pes buffer
  //}}}
  //{{{
  ~cDecoderQueueItem() {
    // release malloc'd pes buffer
    free (mPes);
    }
  //}}}

  cDecoder* mDecoder;
  uint8_t* mPes;
  const int mPesSize;
  const int64_t mPts;
  const int64_t mDts;
  const std::function<void (cFrame* frame)> mAddFrameCallback;
  };
//}}}

class cRender {
public:
  enum eDecoder { eFFmpeg, eMfxSystem, eMfxVideo };

  cRender (bool useQueue, const std::string name, uint8_t streamType, uint16_t decoderMask);
  virtual ~cRender();

  bool isQueued() const { return mQueued; }

  std::shared_mutex& getSharedMutex() { return mSharedMutex; }
  uint8_t getStreamType() const { return mStreamType; }

  // miniLog
  cMiniLog& getLog() { return mMiniLog; }
  void toggleLog();

  // value log
  float getValue (int64_t pts) const;
  float getOffsetValue (int64_t ptsOffset, int64_t& pts) const;

  int64_t getLastPts() const { return mLastPts; }
  size_t getNumValues() const { return mValuesMap.size(); }

  void setRefPts (int64_t pts) { mRefPts = pts; }
  void setMapSize (size_t size) { mMapSize = size; }
  void logValue (int64_t pts, float value);

  // process
  virtual std::string getInfo() const = 0;
  virtual void addFrame (cFrame* frame) = 0;
  virtual bool processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) = 0;

protected:
  void header();
  void log (const std::string& tag, const std::string& text);

  std::shared_mutex mSharedMutex;
  cDecoder* mDecoder = nullptr;
  std::function <void (cFrame* frame)> mAddFrameCallback;

  // decode queue
  void dequeThread();
  void exitThread();
  int getQueueSize() const;
  float getQueueFrac() const;

  bool mQueueExit = false;
  bool mQueueRunning = false;
  readerWriterQueue::cBlockingReaderWriterQueue <cDecoderQueueItem*> mQueue;

private:
  const bool mQueued = false;
  const std::string mName;
  const uint8_t mStreamType;
  const uint16_t mDecoderMask;

  cMiniLog mMiniLog;

  // plot
  std::map <int64_t,float> mValuesMap;
  int64_t mLastPts = 0;
  int64_t mRefPts = 0;
  size_t mMapSize = 64;
  };

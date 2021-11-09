// cRender.h - stream render base class
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <mutex>
#include <shared_mutex>

#include "../utils/cMiniLog.h"

class cFrame;
class cDecoder;
//}}}
constexpr int kPtsPerSecond = 90000;

class cRender {
public:
  cRender (const std::string name, uint8_t streamType);
  virtual ~cRender();

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
  virtual std::string getInfoString() const = 0;
  virtual void addFrame (cFrame* frame) = 0;
  virtual void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) = 0;

protected:
  void header();
  void log (const std::string& tag, const std::string& text);

  std::shared_mutex mSharedMutex;
  cDecoder* mDecoder = nullptr;

private:
  const std::string mName;
  const uint8_t mStreamType;
  cMiniLog mMiniLog;

  // plot
  std::map <int64_t,float> mValuesMap;
  int64_t mLastPts = 0;
  int64_t mRefPts = 0;
  size_t mMapSize = 64;
  };

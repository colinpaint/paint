// cDecoder.h
//{{{  includes
#pragma once

#include "cRender.h"

#include <cstdint>
#include <string>
#include <map>

#include "../utils/cMiniLog.h"
//}}}
constexpr int kPtsPerSecond = 90000;

class cRender {
public:
  cRender (const std::string name);
  virtual ~cRender();

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
  virtual void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts) = 0;

protected:
  void header();
  void log (const std::string& tag, const std::string& text);

private:
  const std::string mName;
  cMiniLog mMiniLog;

  // plot
  std::map <int64_t,float> mValuesMap;
  int64_t mLastPts = 0;
  int64_t mRefPts = 0;
  size_t mMapSize = 64;
  };

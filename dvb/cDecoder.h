// cDecoder.h
//{{{  includes
#pragma once

#include "cDecoder.h"

#include <cstdint>
#include <string>
#include <map>

#include "../utils/cMiniLog.h"
//}}}

class cDecoder {
public:
  cDecoder (const std::string name);
  virtual ~cDecoder();

  // miniLog
  cMiniLog& getLog() { return mMiniLog; }
  void toggleLog();

  // value log
  int64_t getLastPts() const { return mLastPts; }
  float getValue (int64_t pts) const;
  void setMapSize (size_t size) { mMapSize = size; }
  void logValue (int64_t pts, float value);

  // decode
  virtual bool decode (const uint8_t* buf, int bufSize, int64_t pts) = 0;

protected:
  void header();
  void log (const std::string& tag, const std::string& text);

private:
  const std::string mName;
  cMiniLog mMiniLog;

  // plot
  std::map <int64_t,float> mValuesMap;
  float mMaxValue = 0.f;
  int64_t mLastPts = 0;
  size_t mMapSize = 64;
  };

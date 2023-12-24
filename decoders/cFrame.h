// cFrame.h - mpeg stransport stream frame base class
//{{{  includes
#pragma once
#include <cstdint>
#include <string>

#include "../common/cLog.h"
//}}}
constexpr bool kFrameDebug = false;

// base class
class cFrame {
public:
  virtual ~cFrame() {
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFrame::~cFrame"));
    }

  int64_t getPts() const { return mPts; }
  int64_t getPtsDuration() const { return mPtsDuration; }
  int64_t getStreamPos() const { return mStreamPos; }
  uint32_t getPesSize() const { return mPesSize; }

  void setPts (int64_t pts) { mPts = pts; }
  void setPtsDuration (int64_t ptsDuration) { mPtsDuration = ptsDuration; }
  void setStreamPos (int64_t streamPos) { mStreamPos = streamPos; }
  void setPesSize (uint32_t pesSize) { mPesSize = pesSize; }

  virtual void releaseResources() = 0;

private:
  int64_t mPts = 0;
  int64_t mPtsDuration = 0;
  int64_t mStreamPos = 0;
  uint32_t mPesSize = 0;
  };

// cFrame.h - stream render base class
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
  cFrame()  = default;
  virtual ~cFrame() {
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFrame::~cFrame"));
    }

  int64_t getPts() const { return mPts; }
  int64_t getPtsDuration() const { return mPtsDuration; }
  int64_t getPesSize() const { return mPesSize; }

  void setPts (int64_t pts) { mPts = pts; }
  void setPtsDuration (int64_t ptsDuration) { mPtsDuration = ptsDuration; }
  void setPesSize (uint32_t pesSize) { mPesSize = pesSize; }

  virtual void releaseResources() = 0;

private:
  int64_t mPts = 0;
  int64_t mPtsDuration = 0;
  uint32_t mPesSize = 0;
  };

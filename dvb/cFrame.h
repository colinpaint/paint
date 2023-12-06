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
  cFrame() {}

  virtual ~cFrame() {
    if (kFrameDebug)
      cLog::log (LOGINFO, fmt::format ("cFrame::~cFrame"));
    }

  int64_t getPts() const { return mPts; }
  int64_t getPtsDuraction() const { return mPtsDuration; }
  int64_t getPesSize() const { return mPesSize; }

  virtual void releaseResources() = 0;

  // vars
  int64_t mPts = 0;
  int64_t mPtsDuration = 0;

  uint32_t mPesSize = 0;
  };

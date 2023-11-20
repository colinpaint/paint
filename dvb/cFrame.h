// cFrame.h - stream render base class
#pragma once
#include <cstdint>
#include "../common/cLog.h"

// base class
class cFrame {
public:
  cFrame() {}
  virtual ~cFrame() {
    cLog::log (LOGINFO, fmt::format ("cFrame::~cFrame"));
    }

  virtual void releaseResources() = 0;

  // vars
  int64_t mPts;
  int64_t mPtsDuration;
  uint32_t mPesSize;
  };

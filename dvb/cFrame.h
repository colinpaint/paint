// cFrame.h - frame base class
//{{{  includes
#pragma once

#include <cstdint>
//}}}

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

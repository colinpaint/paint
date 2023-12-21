// cBaseOptions.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

#include "../common/cLog.h"
//}}}

constexpr int kPtsPerSecond = 90000;
constexpr int64_t kPtsPer25HzFrame = kPtsPerSecond / 25;

class iOptions {
public:
  virtual ~iOptions() = default;
  };

// cVideoDecoder.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include "../utils/cMiniLog.h"

//}}}

class cVideoDecoder {
public:
  cVideoDecoder (const std::string name);
  ~cVideoDecoder();

  cMiniLog& getLog() { return mMiniLog; }
  void toggleLog();

  bool decode (const uint8_t* buf, int bufSize, int64_t pts);

private:
  void header();
  void log (const std::string& tag, const std::string& text);

  // vars
  const std::string mName;
  cMiniLog mMiniLog;
  };

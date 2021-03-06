// utils.h - simple utils using format and string
#pragma once
#include <cstdint>
#include <string>
#include "formatCore.h"
//#include "date.h"

//{{{
inline std::string getTimeString (int64_t value, int daylightSeconds) {

  int64_t subSeconds = (value % 100);
  value /= 100;

  value += daylightSeconds;
  int64_t seconds = value % 60;
  value /= 60;

  int64_t minutes = value % 60;
  value /= 60;

  int64_t hours = value;

  if (hours > 0)
    return fmt::format ("{}:{:02d}:{:02d}:{:02d}", hours, minutes, seconds, subSeconds);
  else if (minutes > 0)
    return fmt::format ("{}:{:02d}:{:02d}", minutes, seconds, subSeconds);
  else
    return fmt::format ("{}:{:02d}", seconds, subSeconds);
  }
//}}}

//{{{
inline std::string getPtsString (int64_t pts) {
// 90khz int64_t pts - miss out zeros

  if (pts < 0)
    return "--:--:--";
  else {
    pts /= 900;
    uint32_t hs = pts % 100;

    pts /= 100;
    uint32_t secs = pts % 60;

    pts /= 60;
    uint32_t mins = pts % 60;

    pts /= 60;
    uint32_t hours = pts % 60;


    std::string hourMinString = hours ? fmt::format ("{}:{:02}", hours,mins) : fmt::format ("{}",mins);
    return fmt::format ("{}:{:02d}:{:02d}", hourMinString, secs, hs);
    }
  }
//}}}
//{{{
inline std::string getFullPtsString (int64_t pts) {
// 90khz int64_t pts  - all digits

  if (pts < 0)
    return "--:--:--:--";
  else {
    pts /= 900;
    uint32_t hs = pts % 100;

    pts /= 100;
    uint32_t secs = pts % 60;

    pts /= 60;
    uint32_t mins = pts % 60;

    pts /= 60;
    uint32_t hours = pts % 60;

    return fmt::format ("{:02d}:{:02d}:{:02d}:{:02d}", hours, mins, secs, hs);
    }
  }
//}}}
//{{{
inline std::string getPtsFramesString (int64_t pts, int64_t ptsDuration) {
// return 90khz int64_t pts - as frames.subframes

  int64_t frames = pts / ptsDuration;
  int64_t subFrames = pts % ptsDuration;
  return fmt::format ("{}.{:04d}", frames,subFrames);
  }
//}}}

//{{{
inline std::string validFileString (const std::string& str, const char* inValidChars) {

  std::string validStr = str;
  #ifdef _WIN32
    for (auto i = 0u; i < strlen(inValidChars); ++i)
      validStr.erase (std::remove (validStr.begin(), validStr.end(), inValidChars[i]), validStr.end());
  #else
    (void)inValidChars;
  #endif

  return validStr;
  }
//}}}

// utils.h
#pragma once
#include <cstdint>
#include <string>
#include "formatCore.h"
//#include "date.h"

//{{{
inline std::string getTimeString (uint32_t secs) {
  return fmt::format ("{}:{}:{}", secs / (60 * 60), (secs / 60) % 60, secs % 60);
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

    //std::string str (hours ? (dec (hours) + ':' + dec (mins, 2, '0')) : dec (mins));
    //return str + ':' + dec(secs, 2, '0') + ':' + dec(hs, 2, '0');
    return "";
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

    //return dec (hours,2,'0') + ':' + dec (mins,2,'0') + ':' + dec(secs,2,'0') + ':' + dec(hs,2,'0');
    return "";
    }
  }
//}}}
//{{{
inline std::string getPtsFramesString (int64_t pts, int64_t ptsDuration) {
// return 90khz int64_t pts - as frames.subframes

  int64_t frames = pts / ptsDuration;
  int64_t subFrames = pts % ptsDuration;
  //return dec(frames) + '.' + dec(subFrames, 4, '0');
  return"";
  }
//}}}

//{{{
inline std::string validFileString (const std::string& str, const char* inValidChars) {

  auto validStr = str;
  for (auto i = 0u; i < strlen(inValidChars); ++i)
    validStr.erase (std::remove (validStr.begin(), validStr.end(), inValidChars[i]), validStr.end());

  return validStr;
  }
//}}}

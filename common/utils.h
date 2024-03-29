// utils.h - simple utils using format and string
//{{{  includes
#pragma once
#include <cstdint>

#include "../fmt/include/fmt/format.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #define NOMINMAX
  #include <windows.h>
#endif

#include <string>
//}}}

namespace utils {
  //{{{
  inline std::string getPtsString (int64_t pts) {
  // 90khz int64_t pts - miss out zeros

    if (pts < 0)
      return "--:--:--";
    else {
      pts /= 900;
      uint32_t subSecs = pts % 100;

      pts /= 100;
      uint32_t secs = pts % 60;

      pts /= 60;
      uint32_t mins = pts % 60;

      pts /= 60;
      uint32_t hours = pts % 60;


      std::string hourMinString = hours ? fmt::format ("{}:{:02}", hours,mins) : fmt::format ("{}",mins);
      return fmt::format ("{}:{:02d}:{:02d}", hourMinString, secs, subSecs);
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
      uint32_t subSecs = pts % 100;

      pts /= 100;
      uint32_t secs = pts % 60;

      pts /= 60;
      uint32_t mins = pts % 60;

      pts /= 60;
      uint32_t hours = pts % 60;

      return fmt::format ("{:02d}:{:02d}:{:02d}:{:02d}", hours, mins, secs, subSecs);
      }
    }
  //}}}
  //{{{
  inline std::string getCompletePtsString (int64_t pts) {
  // 90khz int64_t pts  show clock rather than subSecond

    if (pts < 0)
      return "--:--:--:--";
    else {
      uint32_t clocks = pts % 90000;

      pts /= 90000;
      uint32_t secs = pts % 60;

      pts /= 60;
      uint32_t mins = pts % 60;

      pts /= 60;
      uint32_t hours = pts % 60;

      return fmt::format ("{:02d}:{:02d}:{:02d}.{:05d}", hours, mins, secs, clocks);
      }
    }
  //}}}
  //{{{
  inline std::string getFramesPtsString (int64_t pts, int64_t ptsDuration) {
  // return 90khz int64_t pts - as frames.subframes

    int64_t frames = pts / ptsDuration;
    int64_t subFrames = pts % ptsDuration;
    return fmt::format ("{}.{:04d}", frames,subFrames);
    }
  //}}}

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
  inline std::string getValidFileString (const std::string& str) {

    const char* inValidChars=  "<>:/|?*\"\'\\";

    std::string validStr = str;
    for (auto i = 0u; i < strlen(inValidChars); ++i)
      validStr.erase (std::remove (validStr.begin(), validStr.end(), inValidChars[i]), validStr.end());
    return validStr;
    }
  //}}}

  //{{{
  inline std::wstring stringToWstring (const std::string& str) {
    return std::wstring (str.begin(), str.end());
    }
  //}}}
  #ifdef _WIN32
    //{{{
    inline std::string wstringToString (const std::wstring& input) {

      int required_characters = WideCharToMultiByte (CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()),
                                                     nullptr, 0, nullptr, nullptr);
      if (required_characters <= 0)
        return {};

      std::string output;
      output.resize (static_cast<size_t>(required_characters));
      WideCharToMultiByte (CP_UTF8, 0, input.c_str(), static_cast<int>(input.size()),
                           output.data(), static_cast<int>(output.size()), nullptr, nullptr);

      return output;
      }
    //}}}
  #endif
  }

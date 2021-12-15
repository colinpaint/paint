// cPlatform.cpp
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
  #include <windows.h>
#endif

#include "cPlatform.h"
#include "../utils/cLog.h"

using namespace std;

//{{{
chrono::system_clock::time_point cPlatform::now() {
// get time_point with daylight saving correction
// - should be a C++20 timezone thing, but not yet

  // get daylight saving flag
  time_t current_time;
  time (&current_time);
  struct tm* timeinfo = localtime (&current_time);
  //cLog::log (LOGINFO, fmt::format ("dst {}", timeinfo->tm_isdst));

  // UTC->BST only
  return chrono::system_clock::now() + chrono::hours ((timeinfo->tm_isdst == 1) ? 1 : 0);
  }
//}}}

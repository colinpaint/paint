// cMiniLog - mini log class, used by in class GUI loggers
//{{{  includes
#include "cMiniLog.h"

#include <cstdint>
#include <string>
#include <vector>
#include <deque>

#include <chrono>

#include "formatCore.h"
#include "date.h"
#include "cLog.h"

using namespace std;
//}}}

//{{{
void cMiniLog::setEnable (bool enable) {

  if (enable != mEnable) {
    if (enable)
      mLog.push_back (fmt::format("{} subtitle log level:{}", mName, mLevel));
    else
      mLog.clear();

    mEnable = enable;
    }
  }
//}}}
//{{{
void cMiniLog::setLevel (uint8_t level) {

  if (level != mLevel) {
    mLevel = level;
    mLog.push_back (fmt::format("{} subtitle log level:{}", mName, mLevel));
    }
  }
//}}}
//{{{
void cMiniLog::toggleEnable() {
  setEnable (!mEnable);
  }
//}}}

//{{{
void cMiniLog::log (const std::string& text, uint8_t level) {

  if (mEnable && (level <= mLevel)) {
    // prepend name for console log
    cLog::log (LOGINFO, mName + text);

    // prepend time for gui window log
    mLog.push_back (date::format ("%T ", chrono::floor<chrono::microseconds>(chrono::system_clock::now())) + text);
    }
  }
//}}}

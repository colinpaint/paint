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

cMiniLog::cMiniLog(const string& name) : mName(name), mFirstTimePoint (chrono::system_clock::now()) {}

//{{{
void cMiniLog::setEnable (bool enable) {

  if (enable != mEnable) {
    if (enable)
      mLog.push_back (fmt::format("{} log level:{}", mName, mLevel));
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
void cMiniLog::log (const string& text, uint8_t level) {

  if (mEnable && (level <= mLevel)) {
    // prepend name for console log
    cLog::log (LOGINFO, mName + " " + text);

    // prepend time for gui window log
    chrono::system_clock::time_point now = chrono::system_clock::now();
    mLog.push_back (date::format ("%M.%S ", now - mFirstTimePoint) + text);
    }
  }
//}}}

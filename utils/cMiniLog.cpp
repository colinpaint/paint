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

  mLog.clear();

  if (enable)
    mLog.push_back (fmt::format("{} subtitle log", mName));

  mEnable = enable;
  }
//}}}

//{{{
void cMiniLog::log (const std::string& text) {

  if (mEnable)
    // prepend name for console log
    cLog::log (LOGINFO, mName + text);

    // prepend time for gui window log
    mLog.push_back (date::format ("%T ", chrono::floor<chrono::microseconds>(chrono::system_clock::now())) + text);
  }
//}}}

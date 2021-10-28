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

// !!!! crude tag scheme - needs working up !!!
cMiniLog::cMiniLog (const string& name) : mName(name), mFirstTimePoint (chrono::system_clock::now()) {}

string cMiniLog::getHeader() const { return mName + " " + mHeader; }

//{{{
void cMiniLog::setEnable (bool enable) {

  if (enable != mEnable) {
    mLog.clear();
    mEnable = enable;
    }
  }
//}}}
//{{{
void cMiniLog::toggleEnable() {
  setEnable (!mEnable);
  }
//}}}

//{{{
void cMiniLog::clear() {
  mLog.clear();
  }
//}}}

//{{{
void cMiniLog::log (const string& text) {

  if (mEnable) {
    // look for tag
    size_t pos = text.find (' ');
    if ((pos != 0) && (pos != string::npos)) {
      // use first word as tag
      string tag = text.substr (0, pos);

      bool found = false;
      for (auto& curTag : mTags)
        if (tag == curTag) {
          found = true;
          break;
          }
      if (!found) {
        mTags.push_back (tag);
        mFilters.push_back (false);
        }
      }

    // prepend time for gui window log
    chrono::system_clock::time_point now = chrono::system_clock::now();
    mLog.push_back (date::format ("%M.%S ", now - mFirstTimePoint) + text);

    // prepend name for console log
    cLog::log (LOGINFO, mName + " " + text);
    }
  }
//}}}

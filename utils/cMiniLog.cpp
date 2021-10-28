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
    mLines.clear();
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
  mLines.clear();
  }
//}}}

//{{{
void cMiniLog::log (const string& text) {

  if (mEnable) {
    // look for tag
    string tag;
    size_t pos = text.find (' ');
    if ((pos != 0) && (pos != string::npos)) {
      // use first word as tag
      tag = text.substr (0, pos);

      bool found = false;
      for (auto& curTag : mTags)
        if (tag == curTag.mName) {
          found = true;
          break;
          }
      if (!found) 
        mTags.push_back (cTag (tag));
      }

    mLines.push_back (cLine (tag, text, chrono::system_clock::now() - mFirstTimePoint));

    // prepend name for console log
    cLog::log (LOGINFO, mName + " " + text);
    }
  }
//}}}

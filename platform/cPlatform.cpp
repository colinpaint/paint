// cPlatform.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <string>

#include "cPlatform.h"
#include "../utils/cLog.h"

using namespace std;
//}}}

// static - public
//{{{
cPlatform& cPlatform::createByName (const string& name, const cPoint& windowSize,
                                    bool showViewports, bool vsync, bool fullScreen) {

  cPlatform* platform = getClassRegister()[name](name);
  if (!platform) {
    cLog::log (LOGERROR, fmt::format ("platform create failed - {} not found", name));
    exit (EXIT_FAILURE);
    }

  platform->init (windowSize, showViewports, vsync, fullScreen);
  return *platform;
  }
//}}}
//{{{
void cPlatform::listRegisteredClasses() {
  cLog::log (LOGINFO, "platform register");
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, fmt::format ("- {}", ui.first));
  }
//}}}

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

// static - protected
//{{{
bool cPlatform::registerClass (const string& name, const createFunc factoryMethod) {

  if (getClassRegister().find (name) == getClassRegister().end()) {
    // className not found - add to classRegister map
    getClassRegister().insert (make_pair (name, factoryMethod));
    return true;
    }
  else
    return false;
  }
//}}}
//{{{
map<const string, cPlatform::createFunc>& cPlatform::getClassRegister() {
// static map inside static method ensures map is created before any use

  static map<const string, createFunc> mClassRegistry;
  return mClassRegistry;
  }
//}}}

// cPlatform.cpp
//{{{  includes
#include <cstdint>
#include <string>

#include "cPlatform.h"
#include "../utils/cLog.h"

using namespace std;
using namespace fmt;
//}}}

// static - public
//{{{
cPlatform& cPlatform::createByName (const string& name, const cPoint& windowSize, bool showViewports) {

  cPlatform* platform = getClassRegister()[name](name);
  if (!platform) {
    cLog::log (LOGERROR, format ("platform create failed - {} not found", name));
    exit (EXIT_FAILURE);
    }

  platform->init (windowSize, showViewports);
  return *platform;
  }
//}}}
//{{{
void cPlatform::listRegisteredClasses() {
  cLog::log (LOGINFO, "platform register");
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, format ("- {}", ui.first));
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

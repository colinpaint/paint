// cPlatform.cpp
//{{{  includes
#include <cstdint>
#include <string>

#include "cPlatform.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

//{{{
void cPlatform::listClasses() {
  cLog::log (LOGINFO, "platform register");
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, format ("- {}", ui.first));
  }
//}}}
//{{{
cPlatform& cPlatform::createByName (const string& name) {
  return *getClassRegister()[name](name);
  }
//}}}

//{{{
bool cPlatform::registerClass (const string& name, const createFunc factoryMethod) {
// trickery - function needs to be called by a derived class inside a static context

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
// trickery - static map inside static method ensures map is created before any use
  static map<const string, createFunc> mClassRegistry;
  return mClassRegistry;
  }
//}}}

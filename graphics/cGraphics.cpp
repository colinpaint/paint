// cGraphics.cpp - graphics factory create
//{{{  includes
#include "cGraphics.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include "../utils/cLog.h"

using namespace std;
//}}}

// public
//{{{
cGraphics& cGraphics::createByName (const string& name, cPlatform& platform) {

  cGraphics* graphics = getClassRegister()[name](name);
  if (!graphics) {
    cLog::log (LOGERROR, format ("graphics create failed - {} not found", name));
    exit (EXIT_FAILURE);
    }

  graphics->init (platform);
  return *graphics;
  }
//}}}
//{{{
void cGraphics::listRegisteredClasses() {

  cLog::log (LOGINFO, "graphics register");
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, format ("- {}", ui.first));
  }
//}}}

// protected
//{{{
bool cGraphics::registerClass (const string& name, const createFunc factoryMethod) {

  if (getClassRegister().find (name) == getClassRegister().end()) {
    // className not found - add to classRegister map
    getClassRegister().insert (make_pair (name, factoryMethod));
    return true;
    }
  else
    return false;
  }
//}}}

// staic private
//{{{
map<const string, cGraphics::createFunc>& cGraphics::getClassRegister() {
// static map inside static method ensures map is created before any use

  static map<const string, createFunc> mClassRegistry;
  return mClassRegistry;
  }
//}}}

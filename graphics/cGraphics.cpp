// cGraphics.cpp - graphics factory create
//{{{  includes
#include "cGraphics.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

// public
//{{{
cGraphics& cGraphics::createByName (const string& name) {
  return *getClassRegister()[name](name);
  }
//}}}
//{{{
void cGraphics::listClasses() {

  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, format ("graphics - {}", ui.first));
  }
//}}}

// protected
//{{{
bool cGraphics::registerClass (const string& name, const createFunc factoryMethod) {
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

// private
//{{{
map<const string, cGraphics::createFunc>& cGraphics::getClassRegister() {
// trickery - static map inside static method ensures map is created before any use

  static map<const string, createFunc> mClassRegistry;
  return mClassRegistry;
  }
//}}}

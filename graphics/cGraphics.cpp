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
cGraphics& cGraphics::createByName (const string& name, void* device, void* deviceContext, void* swapChain) {

  cGraphics* graphics = getClassRegister()[name](name);
  if (!graphics) {
    cLog::log (LOGERROR, format ("graphics create failed - {} not found", name));
    exit (EXIT_FAILURE);
    }

  graphics->init (device, deviceContext, swapChain);
  return *graphics;
  }
//}}}
//{{{
void cGraphics::listClasses() {

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

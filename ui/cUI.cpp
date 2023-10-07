// cUI.cpp - static manager and UI base class
//{{{  includes
#include "cUI.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

// imGui
#include <imgui.h>

// imGui - internal, exposed for custom widget
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include "../app/cGraphics.h"

#include "../common/cLog.h"

using namespace std;
//}}}
#define DRAW_CANVAS // useful to disable canvas when bringing up backends
#define SHOW_DEMO

// public
// - static register
//{{{
cUI* cUI::createByName (const string& name) {
// create class by name from classRegister, add instance to instances

  auto uiIt = getInstanceRegister().find (name);
  if (uiIt == getInstanceRegister().end()) {
    auto ui = getClassRegister()[name](name);
    getInstanceRegister().insert (make_pair (name, ui));
    return ui;
    }
  else
    return uiIt->second;
  }
//}}}
//{{{
void cUI::listRegisteredClasses() {

  cLog::log (LOGINFO, "ui register");
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, fmt::format ("- {}", ui.first));
  }
//}}}
//{{{
void cUI::listInstances() {

  cLog::log (LOGINFO, "ui instance register");
  for (auto& ui : getInstanceRegister())
    cLog::log (LOGINFO, fmt::format ("- {}", ui.first));
  }
//}}}

// - static render
//{{{
void cUI::render (cApp& app) {
// render registered UI instances

  for (auto& ui : getInstanceRegister())
    ui.second->addToDrawList (app);
  }
//}}}

// protected
// - static manager
//{{{
bool cUI::registerClass (const string& name, const cUI::createFuncType createFunc) {
// register class createFunc by name to classRegister, add instance to instances

  if (getClassRegister().find (name) == getClassRegister().end()) {
    // class name not found - add to classRegister map
    getClassRegister().insert (make_pair (name, createFunc));

    // create instance of class and add to instances map
    getInstanceRegister().insert (make_pair (name, createFunc (name)));
    return true;
    }
  else
    return false;
  }
//}}}

// private:
//{{{
map<const string, cUI::createFuncType>& cUI::getClassRegister() {
// static map inside static method ensures map is created before use

  static map<const string, createFuncType> mClassRegister;
  return mClassRegister;
  }
//}}}
//{{{
map<const string, cUI*>& cUI::getInstanceRegister() {
// static map inside static method ensures map is created before use

  static map<const string, cUI*> mInstances;
  return mInstances;
  }
//}}}

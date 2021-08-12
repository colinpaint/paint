// cBrushMan.cpp - brush manager
// - brushes registered by name
// - select curBrush by name
//{{{  includes
#include "cBrushMan.h"
#include "cBrush.h"

#include <cstdint>
#include <string>

#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

cBrush* cBrushMan::createByName (const std::string& name, float radius) {
  return getClassRegister()[name](name, radius);
  }

bool cBrushMan::registerClass (const std::string& name, const createFunc factoryMethod) {
// trickery - function needs to be called by a derived class inside a static context

  if (getClassRegister().find (name) == getClassRegister().end()) {
    // className not found - add to classRegister map
    getClassRegister().insert (std::make_pair (name, factoryMethod));
    return true;
    }
  else {
    // className found - error
    cLog::log (LOGERROR, fmt::format ("cBrushMan - {} - already registered", name));
    return false;
    }
  }


bool cBrushMan::isCurBrushByName (const std::string& name) {
  return mCurBrush ? name == mCurBrush->getName() : false;
  }

cBrush* cBrushMan::setCurBrushByName (const std::string& name, float radius) {
  if (!isCurBrushByName (name)) {
    // default color if no curBrush
    glm::vec4 color (0.7f,0.7f,0.f, 1.f);

    if (mCurBrush)
      color = mCurBrush->getColor();
    delete mCurBrush;

    mCurBrush = createByName (name, radius);
    mCurBrush->setColor (color);
    cLog::log (LOGINFO, "setCurBrushByName " + name);
    }

  return mCurBrush;
  }

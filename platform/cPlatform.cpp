// cGlfwPlatform.cpp
//{{{  includes
#include <cstdint>
#include <string>

#include "cPlatform.h"
#include "../graphics/cPointRect.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

void cPlatform::listClasses() {
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, format ("platform - {}", ui.first));
  }

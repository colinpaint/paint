// cGraphics.cpp - graphics factory create
#include "cGraphics.h"

#include <cstdint>
#include <cmath>
#include <string>
#include <algorithm>

#include "../log/cLog.h"

using namespace std;
using namespace fmt;

//{{{
void cGraphics::listClasses() {
  for (auto& ui : getClassRegister())
    cLog::log (LOGINFO, format ("graphics - {}", ui.first));
  }
//}}}

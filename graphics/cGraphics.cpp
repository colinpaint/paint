// cGraphics.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <cmath>

#include "cOpenGlGraphics.h"
#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

cGraphics& cGraphics::create() {
  return *(new cOpenGlGraphics());
  }

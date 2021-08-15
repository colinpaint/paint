// cGraphics.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include <cstdint>
#include <cmath>

#include "cOpenGlGraphics.h"
#include "cDirectX11Graphics.h"

#include "../log/cLog.h"

using namespace std;
using namespace fmt;
//}}}

#ifdef WIN32
  // factory create
  cGraphics& cGraphics::create() {
    static cGraphics* graphics = new cDirectX11Graphics();
    return *graphics;
    }
#else
  //{{{
  // factory create
  cGraphics& cGraphics::create() {
    static cGraphics* graphics = new cOpenGlGraphics();
    return *graphics;
    }
  //}}}
#endif

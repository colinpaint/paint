// cPlatform.cpp - platform factory create
//includes
#include "cWin32Platform.h"
#include "cGlfwPlatform.h"

// factory create
cPlatform& cPlatform::create() {
  #ifdef WIN32
    static cPlatform* platform = new cWin32Platform();
  #else
    static cPlatform* platform = new cGlfwPlatform();
  #endif
  return *platform;
  }

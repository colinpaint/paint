// cPlatform.cpp - platform factory create
#include <string>
#include "cWin32Platform.h"
#include "cGlfwPlatform.h"

// static factory create
cPlatform& cPlatform::create (const std::string& selectString) {
  #ifdef WIN32
    // windows offers glfw or win32
    if (selectString == "directx")
      return *new cWin32Platform();
    else
      return *new cGlfwPlatform();
  #else
    return * new cGlfwPlatform();
  #endif
  }

// cPlatform.cpp - platform factory create
//includes
#include <string>
#include "cWin32Platform.h"
#include "cGlfwPlatform.h"

// factory create
cPlatform& cPlatform::create (const std::string& select) {
  #ifdef WIN32
    if (select == "directx") 
      return *new cWin32Platform();
    else
      return *new cGlfwPlatform();
  #else
    return * new cGlfwPlatform();
  #endif
  }

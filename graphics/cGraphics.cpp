// cGraphics.cpp - graphics factory create
#include <string>
#include "cDirectX11Graphics.h"
#include "cOpenGlGraphics.h"

// static factory create
cGraphics& cGraphics::create (const std::string& selectString) {
  #ifdef WIN32
    // windows offers openGl or directX11
    if (selectString == "directx")
      return *new cDirectX11Graphics();
    else
      return *new cOpenGlGraphics();
  #else
    return *new cOpenGlGraphics();
  #endif
  }

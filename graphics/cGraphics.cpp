// cGraphics.cpp - graphics factory create
#include <string>
#include "cOpenGlGraphics.h"
#include "cDirectX11Graphics.h"

// factory create
cGraphics& cGraphics::create (const std::string& select) {
  #ifdef WIN32
    if (select == "directx") 
      return *new cDirectX11Graphics();
    else
      return *new cOpenGlGraphics();
  #else
    return *new cOpenGlGraphics();
  #endif
  }

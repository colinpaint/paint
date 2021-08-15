// cGraphics.cpp - graphics factory create
#include "cOpenGlGraphics.h"
#include "cDirectX11Graphics.h"

// factory create
cGraphics& cGraphics::create() {
  #ifdef WIN32
    static cGraphics* graphics = new cDirectX11Graphics();
  #else
    static cGraphics* graphics = new cOpenGlGraphics();
  #endif

  return *graphics;
  }

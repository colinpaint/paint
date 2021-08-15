project (graphics)

file (GLOB HEADER_FILES *.h)

if (GRAPHICS STREQUAL "DIRECTX11")
  file (GLOB HEADER_FILES cPointRect.h cGraphics.h cDirectX11Graphics.h)
  file (GLOB SRCFILES cGraphics.cpp  cDirectX11Graphics.cpp)
else()
  file (GLOB HEADER_FILES cPointRect.h cGraphics.h cOpenGLGraphics.h)
  file (GLOB SRCFILES cGraphics.cpp  cOpenGLGraphics.cpp)
endif()

add_library (graphics ${SRCFILES} ${HEADER_FILES})

project (graphics)

if (WIN32)
  file (GLOB HEADER_FILES cPointRect.h cGraphics.h cOpenGLGraphics.h cDirectX11Graphics.h)
  file (GLOB SRCFILES cGraphics.cpp cOpenGLGraphics.cpp cDirectX11Graphics.cpp)
else()
  file (GLOB HEADER_FILES cPointRect.h cGraphics.h cOpenGLGraphics.h)
  file (GLOB SRCFILES cGraphics.cpp cOpenGLGraphics.cpp)
endif()

add_library (graphics ${SRCFILES} ${HEADER_FILES})

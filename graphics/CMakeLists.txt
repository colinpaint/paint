project (graphics)

file (GLOB HEADER_FILES *.h)

if (WIN32)
  file (GLOB SRCFILES cGraphics.cpp
                      cOpenGlGraphics.cpp
                      cDirectX11Graphics.cpp
                      )
else()
  file (GLOB SRCFILES cGraphics.cpp
                      cOpenGlGraphics.cpp
                      )
endif()

add_library (graphics ${SRCFILES} ${HEADER_FILES})

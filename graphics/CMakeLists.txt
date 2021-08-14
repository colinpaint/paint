project (graphics)

file (GLOB HEADER_FILES *.h)

if (GRAPHICS STREQUAL "DIRECTX11")
  file (GLOB SRCFILES cGraphics-DirectX11.cpp)
else()
  file (GLOB SRCFILES cGraphics-OpenGL.cpp)
endif()

add_library (graphics ${SRCFILES} ${HEADER_FILES})

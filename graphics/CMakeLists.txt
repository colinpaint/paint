project (graphics)

file (GLOB HEADER_FILES *.h)

if (GRAPHICS STREQUAL "DIRECTX11")
  file (GLOB SRCFILES *DirectX11.cpp)
else()
  file (GLOB SRCFILES *OpenGL.cpp)
endif()

add_library (graphics ${SRCFILES} ${HEADER_FILES})

# cmake config
cmake_minimum_required (VERSION 3.10)
cmake_policy (SET CMP0072 NEW)

# project name
project (paintbox)

# config
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

set (OPENGL_VERSION "OPENGL_40")

# compiler,linker flags
if (WIN32)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /arch:AVX")
else()
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
endif()

# openGL versions
if (OPENGL_VERSION STREQUAL "OPENGL_21")
  # openGL 2.1 with fbo vbo extensions
  include_directories (${CMAKE_SOURCE_DIR}/glad2.1+fbo+vao/include
                       ${CMAKE_SOURCE_DIR}/glfw/include
                       ${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/glm ${CMAKE_SOURCE_DIR}/stb)
  add_subdirectory (glad2.1+fbo+vao/)
else()
  # openGL >= 3.0
  include_directories (${CMAKE_SOURCE_DIR}/glad4.5core/include
                       ${CMAKE_SOURCE_DIR}/glfw/include
                       ${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/glm ${CMAKE_SOURCE_DIR}/stb)
  add_subdirectory (glad4.5core/)
endif()

# subdirectories
find_package (OpenGL REQUIRED)
set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory (glfw/)
add_subdirectory (canvas/)
add_subdirectory (tinyfiledialog/)
add_subdirectory (imgui/)
add_subdirectory (utils/)

# main + static init registered, ??? TU problem in library ???
file (GLOB SRCFILES
  ${CMAKE_SOURCE_DIR}/main.cpp
  ${CMAKE_SOURCE_DIR}/platform/*.cpp
  ${CMAKE_SOURCE_DIR}/graphics/*.cpp
  ${CMAKE_SOURCE_DIR}/brush/*.cpp
  ${CMAKE_SOURCE_DIR}/ui/*.cpp
  )
file (GLOB HEADER_FILES
  ${CMAKE_SOURCE_DIR}/canvas/*.h
  ${CMAKE_SOURCE_DIR}/tinyfiledialog/*.h
  ${CMAKE_SOURCE_DIR}/platform/*.h
  ${CMAKE_SOURCE_DIR}/graphics/*.h
  ${CMAKE_SOURCE_DIR}/log/*.h
  ${CMAKE_SOURCE_DIR}/brush/*.h
  ${CMAKE_SOURCE_DIR}/ui/*.h
  )

# main executable
add_executable (paintbox ${SRCFILES} ${HEADER_FILES})

# link, watch out for order dependent static link in linux
if (WIN32)
  target_link_libraries (paintbox canvas tinyfiledialog imgui utils glad glfw opengl32 d3d11 d3dcompiler)
else()
  target_link_libraries (paintbox canvas tinyfiledialog imgui utils glad glfw GL)
endif()

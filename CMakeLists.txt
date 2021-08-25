# cmake config
cmake_minimum_required (VERSION 3.10)
cmake_policy (SET CMP0072 NEW)

# project name
project (paintbox VERSION 1.0.0)

# config
#set (USE_IMPLOT)
set (OPENGL_VERSION "OPENGL_45")
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

find_package (OpenGL REQUIRED)

# executable
# - main + static registered .cpp
file (GLOB SOURCE_FILES main.cpp platform/*.cpp graphics/*.cpp brush/*.cpp ui/*.cpp)
add_executable (paintbox ${SOURCE_FILES})

# target
if (OPENGL_VERSION STREQUAL "OPENGL_21")
  add_subdirectory (glad2.1+fbo+vao)
elseif (OPENGL_VERSION STREQUAL "OPENGL_45")
  add_subdirectory (glad4.5core)
endif()

if (USE_IMPLOT)
  # optioned as implot compiles very slowly
  add_subdirectory (implot)
  target_compile_definitions (paintbox PUBLIC USE_IMPLOT)
  target_link_libraries (paintbox PUBLIC implot)
endif()

set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory (glfw)
add_subdirectory (utils)
add_subdirectory (imgui)
add_subdirectory (canvas)

target_compile_features (paintbox PUBLIC cxx_std_17)
target_compile_definitions (paintbox PUBLIC OPENGL_VERSION)
target_include_directories (paintbox PUBLIC glfw/include stb)
target_link_libraries (paintbox PUBLIC canvas imgui utils glfw glad)

if (MSVC)
  target_compile_options (paintbox PUBLIC /W4 /arch:AVX /fp:fast)
  target_link_libraries (paintbox PUBLIC opengl32 d3d11 d3dcompiler)
else()
  target_compile_options (paintbox PUBLIC -Wall -Wextra -pedantic -Ofast)
  target_link_libraries (paintbox PUBLIC GL)
  target_link_options (paintbox PUBLIC -no-pie)
endif()

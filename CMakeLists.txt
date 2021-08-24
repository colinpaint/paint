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
target_compile_features (paintbox PRIVATE cxx_std_17)
target_include_directories (paintbox PUBLIC glfw/include stb)

target_compile_definitions (paintbox PUBLIC OPENGL_VERSION)
if (OPENGL_VERSION STREQUAL "OPENGL_21")
  include_directories (glad2.1+fbo+vao/include)
  add_subdirectory (glad2.1+fbo+vao)
  target_include_directories (paintbox PUBLIC glad2.1+fbo+vao/include)
else()
  include_directories (glad4.5core/include)
  add_subdirectory (glad4.5core)
  target_include_directories (paintbox PUBLIC glad4.5core/include)
endif()

if (USE_IMPLOT)
  add_subdirectory (implot)
  target_link_libraries (paintbox implot)
  target_compile_definitions (paintbox PUBLIC USE_IMPLOT)
endif()

add_subdirectory (canvas)
add_subdirectory (utils)
add_subdirectory (imgui)
set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory (glfw)

target_link_libraries (paintbox canvas imgui utils glfw glad)
# target platform
if (MSVC)
  target_compile_options (paintbox PUBLIC /W4 /arch:AVX /fp:fast)
  target_link_libraries (paintbox opengl32 d3d11 d3dcompiler)
else()
  target_compile_options (paintbox PUBLIC -Wall -Wextra -pedantic -Ofast)
  target_link_options (paintbox PUBLIC -no-pie)
  target_link_libraries (paintbox GL)
endif()

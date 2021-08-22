# cmake config
cmake_minimum_required (VERSION 3.10)
cmake_policy (SET CMP0072 NEW)

# project name
project (paintbox VERSION 1.0.0)

# config
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

# openGL versions
set (OPENGL_VERSION "OPENGL_40")
if (OPENGL_VERSION STREQUAL "OPENGL_21")
  # openGL 2.1 with fbo vbo extensions
  include_directories (${CMAKE_SOURCE_DIR}/glad2.1+fbo+vao/include
                       ${CMAKE_SOURCE_DIR}/glfw/include
                       ${CMAKE_SOURCE_DIR}/imgui
                       ${CMAKE_SOURCE_DIR}/implot
                       ${CMAKE_SOURCE_DIR}/stb)
  add_subdirectory (glad2.1+fbo+vao/)
else()
  include_directories (${CMAKE_SOURCE_DIR}/glad4.5core/include
                       ${CMAKE_SOURCE_DIR}/glfw/include
                       ${CMAKE_SOURCE_DIR}/imgui
                       ${CMAKE_SOURCE_DIR}/implot
                       ${CMAKE_SOURCE_DIR}/stb)
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
add_subdirectory (imgui/)
add_subdirectory (implot/)
add_subdirectory (utils/)

# main + static init registered .cpp
file (GLOB SRCFILES
  ${CMAKE_SOURCE_DIR}/main.cpp
  ${CMAKE_SOURCE_DIR}/platform/*.cpp
  ${CMAKE_SOURCE_DIR}/graphics/*.cpp
  ${CMAKE_SOURCE_DIR}/brush/*.cpp
  ${CMAKE_SOURCE_DIR}/ui/*.cpp
  )
file (GLOB HEADER_FILES
  ${CMAKE_SOURCE_DIR}/utils/*.h
  ${CMAKE_SOURCE_DIR}/canvas/*.h
  ${CMAKE_SOURCE_DIR}/platform/*.h
  ${CMAKE_SOURCE_DIR}/graphics/*.h
  ${CMAKE_SOURCE_DIR}/log/*.h
  ${CMAKE_SOURCE_DIR}/brush/*.h
  ${CMAKE_SOURCE_DIR}/ui/*.h
  )

# executable
add_executable (paintbox ${SRCFILES} ${HEADER_FILES})

target_compile_features (paintbox PRIVATE cxx_std_17)
if (MSVC)
  target_compile_options (paintbox PRIVATE /W4 /arch:AVX /fp:fast /O2)
  target_link_libraries (paintbox canvas implot imgui utils glad glfw opengl32 d3d11 d3dcompiler)
else()
  # watch out for order dependent static link in linux, no compile options -mavx -Werror
  target_compile_options (paintbox PRIVATE -Wall -Wextra -pedantic -Ofast)
  target_link_options (paintbox PRIVATE -no-pie)
  target_link_libraries (paintbox canvas implot imgui utils glad glfw GL)
endif()

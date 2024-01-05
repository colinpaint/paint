#CXX="clang++" CC="clang" cmake ..
#
cmake_minimum_required (VERSION 3.18)
cmake_policy (SET CMP0072 NEW)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
#
# options - GL3, GLES30,GLES31,GLES32
#cmake .. -DBUILD_GRAPHICS=GLES32
#
if (CMAKE_HOST_WIN32)
  set (BUILD_MSVC_COMPILE_OPTIONS "/arch:AVX" CACHE STRING "msvc compile options")
  set (BUILD_GRAPHICS "GL3" CACHE STRING "graphics backend")
else()
  set (BUILD_LINUX_COMPILE_OPTIONS "-Ofast" CACHE STRING "gcc compile options")
  set (BUILD_GRAPHICS "GLES32" CACHE STRING "graphics backend")
endif()
#
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

# main build type message
message (STATUS "building ${BUILD_GRAPHICS} graphics")

# glad lib
if (BUILD_GRAPHICS STREQUAL "GL3")
  add_subdirectory (glad4.5core)
endif()

# glfw copy lib
set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
#set (GLFW_USE_WAYLAND ON CACHE BOOL "" FORCE)
add_subdirectory (glfw)

# subdirectories
add_subdirectory (date)
add_subdirectory (fmt)
add_subdirectory (common)
add_subdirectory (imgui)
add_subdirectory (app)
add_subdirectory (net)
add_subdirectory (dvb)
add_subdirectory (decoders)
add_subdirectory (audio)
add_subdirectory (song)

# windows audio captureApp
if (CMAKE_HOST_WIN32)
  project (captureApp VERSION 1.0.0)
    add_executable (${PROJECT_NAME} apps/captureApp.cpp)
    target_link_libraries (${PROJECT_NAME} PRIVATE decoders audio winmm avrt)
endif()

# imguiApp
project (imguiApp VERSION 1.0.0)
  add_executable (${PROJECT_NAME} apps/imguiApp.cpp)

  target_include_directories (${PROJECT_NAME} PRIVATE stb)
  if (CMAKE_HOST_WIN32)
    target_link_libraries (${PROJECT_NAME} PRIVATE app opengl32)
  else()
    target_link_libraries (${PROJECT_NAME} PRIVATE app GL)
  endif()

# editApp
project (editApp VERSION 1.0.0)
  add_executable (${PROJECT_NAME} apps/editApp.cpp)

  target_include_directories (${PROJECT_NAME} PRIVATE stb)
  if (CMAKE_HOST_WIN32)
    target_link_libraries (${PROJECT_NAME} PRIVATE app ws2_32)
  else()
    target_link_libraries (${PROJECT_NAME} PRIVATE app stdc++fs)
  endif()

# tellyApp
project (telly VERSION 1.0.0)
  add_executable (${PROJECT_NAME} apps/telly.cpp
                                  apps/cTellyApp.h apps/cTellyApp.cpp
                                  apps/CTellyUI.h apps/cTellyUI.cpp)

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
  target_include_directories (${PROJECT_NAME} PRIVATE stb)
  if (CMAKE_HOST_WIN32)
    target_link_libraries (${PROJECT_NAME} PRIVATE app dvb song net audio ws2_32)
  else()
    target_link_libraries (${PROJECT_NAME} PRIVATE app dvb song net audio stdc++fs pthread)
  endif()

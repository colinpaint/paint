#CXX="clang++" CC="clang" cmake ..
#
cmake_minimum_required (VERSION 3.18)
cmake_policy (SET CMP0072 NEW)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
#
# options - GL3, GLES30,GLES31,GLES32
#cmake .. -DBUILD_GRAPHICS=GLES31
#
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set (BUILD_GRAPHICS "GL3" CACHE STRING "build graphics option")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
  set (BUILD_GRAPHICS "GLES31" CACHE STRING "build graphics option")
endif()
#
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set (BUILD_MSVC_COMPILE_OPTIONS "/arch:AVX" CACHE STRING "msvc compile options")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
  set (BUILD_LINUX_COMPILE_OPTIONS "-Ofast" CACHE STRING "gcc compile options")
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
add_subdirectory (fmt)
add_subdirectory (common)
add_subdirectory (app)
add_subdirectory (imgui)
add_subdirectory (net)
add_subdirectory (dvb)
add_subdirectory (decoders)
add_subdirectory (audio)
add_subdirectory (song)

# windows audio capture app
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  project (capture VERSION 1.0.0)
    add_executable (${PROJECT_NAME} audio/capture.cpp)
    target_link_libraries (${PROJECT_NAME} PRIVATE decoders winmm avrt audio)
endif()

# demo imgui app
project (demo VERSION 1.0.0)
  file (GLOB HEADER demo/*.h ui/*.h imgui/*.h common/*.h app/*.h)
  file (GLOB SOURCE demo/*.cpp ui/cUI.cpp)
  add_executable (demo ${SOURCE} ${HEADER})

  target_include_directories (${PROJECT_NAME} PRIVATE app common stb imgui)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE app imgui opengl32 d3d11 d3dcompiler)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE app imgui GL)
  endif()

# fed app
project (fed VERSION 1.0.0)
  add_executable (${PROJECT_NAME} ui/cUI.h ui/cUI.cpp
                                  fed/cDocument.h fed/cDocument.cpp
                                  fed/cFedApp.h fed/cFedApp.cpp
                                  fed/fedMain.cpp
                                  fed/cEditUI.cpp
                                  fed/cMemEdit.h fed/cMemEdit.cpp
                                  fed/cTextEdit.h fed/cTextEdit.cpp
                                  )

  target_include_directories (${PROJECT_NAME} PRIVATE app imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui stdc++fs)
  endif()

# player app
project (player VERSION 1.0.0)
  add_executable (${PROJECT_NAME} player/cPlayerApp.h
                                  player/playerMain.cpp player/cPlayerUI.cpp player/cPlayerApp.cpp
                                  ui/cUI.h ui/cUI.cpp)

  target_include_directories (${PROJECT_NAME} PRIVATE app imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoders dvb net imgui
                                                   opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoders dvb net imgui
                                                   stdc++fs pthread)
  endif()

# telly app
project (telly VERSION 1.0.0)
  add_executable (${PROJECT_NAME} telly/cTellyApp.h
                                  telly/cTellyApp.cpp telly/cTellyUI.cpp telly/tellyMain.cpp
                                  ui/cUI.h ui/cUI.cpp )

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
  target_include_directories (${PROJECT_NAME} PRIVATE app implot imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio decoders imgui
                                                   opengl32 dxgi d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio decoders imgui
                                                   stdc++fs pthread)
  endif()

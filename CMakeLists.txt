cmake_minimum_required (VERSION 3.18)
cmake_policy (SET CMP0072 NEW)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
#
# options - GL_2_1,GL_3, GLES_3_0,GLES_3_1,GLES_3_2, SDL_GL_3, VULKAN
#cmake .. -DBUILD_GRAPHICS=GLES_3_1
#
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set (BUILD_GRAPHICS "GL_3" CACHE STRING "build graphics option")
  option (BUILD_MFX "Build mfx lib" ON)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
  set (BUILD_GRAPHICS "GLES_3_1" CACHE STRING "build graphics option")
  set (BUILD_MFX OFF)
endif()
#
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set (BUILD_MSVC_COMPILE_OPTIONS "/arch:AVX" CACHE STRING "msvc compile options")
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
  option (BUILD_VALIDATION "Build vulkan validation layers" OFF)
  set (BUILD_LINUX_COMPILE_OPTIONS "-Ofast" CACHE STRING "gcc compile options")
endif()
#
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

# main build type message
message (STATUS "building ${BUILD_GRAPHICS} graphics")
#

# glfw private copy lib
set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
#set (GLFW_USE_WAYLAND ON CACHE BOOL "" FORCE)
add_subdirectory (glfw)

# glad lib
if (BUILD_GRAPHICS STREQUAL "GL_2_1")
  add_subdirectory (glad2.1+fbo+vao)
elseif (BUILD_GRAPHICS STREQUAL "GL_3" OR BUILD_GRAPHICS STREQUAL "SDL_GL_3")
  add_subdirectory (glad4.5core)
endif()

# subdirectories
add_subdirectory (fmt)
add_subdirectory (common)
add_subdirectory (app)
add_subdirectory (imgui)
add_subdirectory (implot)

if (BUILD_MFX AND CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  add_subdirectory (libmfx)
endif()

add_subdirectory (dvb)
add_subdirectory (decoder)
add_subdirectory (kiss)
add_subdirectory (song)
add_subdirectory (net)
add_subdirectory (audio)

# windows audio capture app
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  project (captureAudio VERSION 1.0.0)
    file (GLOB HEADER_FILES audio/*.h common/*.h)
    file (GLOB SOURCE_FILES captureAudio/capture.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE decoder winmm avrt audio)
endif()

# demo imgui app
project (demo VERSION 1.0.0)
  file (GLOB HEADER_FILES demo/*.h ui/*.h imgui/*.h common/*.h app/*.h)
  file (GLOB SOURCE_FILES demo/*.cpp ui/cUI.cpp)
  add_executable (demo ${SOURCE_FILES} ${HEADER_FILES})

  if (GL2)
    target_compile_definitions (${PROJECT_NAME} PRIVATE GL2)
  endif()

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
  target_link_libraries (${PROJECT_NAME} PRIVATE imgui implot)

  target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE GL)
  endif()

# fed app
project (fed VERSION 1.0.0)
  file (GLOB HEADER_FILES fed/*.h ui/*.h imgui/*.h common/*.h app/*.h)
  file (GLOB SOURCE_FILES fed/*.cpp ui/cUI.cpp)
  add_executable (fed ${SOURCE_FILES} ${HEADER_FILES})

  target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui stdc++fs)
  endif()

# player app
project (player VERSION 1.0.0)
  file (GLOB HEADER_FILES player/*.h ui/*.h imgui/*.h common/*.h app/*.h)
  file (GLOB SOURCE_FILES player/playerMain.cpp player/cPlayerUI.cpp player/cPlayerApp.cpp ui/cUI.cpp)
  add_executable (player ${SOURCE_FILES} ${HEADER_FILES})

  target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)
  target_link_libraries (${PROJECT_NAME} PRIVATE imgui song audio decoder dvb kiss net)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
  endif()

# telly app
project (telly VERSION 1.0.0)
  file (GLOB HEADER_FILES telly/*.h ui/*.h imgui/*.h implot/implot.h common/*.h app/*.h)
  file (GLOB SOURCE_FILES telly/tellyMain.cpp telly/cTellyApp.cpp telly/cTellyUI.cpp ui/cUI.cpp)
  add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

  target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
  target_link_libraries (${PROJECT_NAME} PRIVATE implot)

  target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio imgui)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_directories (${PROJECT_NAME} PRIVATE ffmpeg/x64)
    target_link_libraries (${PROJECT_NAME} PRIVATE libmfx opengl32 dxgi d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
  endif()

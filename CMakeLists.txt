cmake_minimum_required (VERSION 3.18)
cmake_policy (SET CMP0072 NEW)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
#
# options - GL_3, GLES_3_0,GLES_3_1,GLES_3_2, SDL_GL_3, VULKAN
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
if (BUILD_GRAPHICS STREQUAL "GL_3" OR BUILD_GRAPHICS STREQUAL "SDL_GL_3")
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

add_subdirectory (net)
add_subdirectory (dvb)
add_subdirectory (decoders)
add_subdirectory (audio)
add_subdirectory (song)

# windows audio capture app
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  project (captureAudio VERSION 1.0.0)
    file (GLOB HEADER audio/*.h common/*.h)
    file (GLOB SOURCE captureAudio/capture.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE} ${HEADER})
    target_link_libraries (${PROJECT_NAME} PRIVATE decoders winmm avrt audio)
endif()

# fed app
project (fed VERSION 1.0.0)
  add_executable (${PROJECT_NAME} fed/cFedApp.h fed/cDocument.h fed/cMemEdit.h fed/cTextEdit.h ui/cUI.h
                                  fed/fedMain.cpp fed/cFedApp.cpp fed/cDocument.cpp
                                  fed/cEditUI.cpp fed/cMemEdit.cpp fed/cTextEdit.cpp ui/cUI.cpp)

  target_include_directories (${PROJECT_NAME} PRIVATE app common imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui stdc++fs)
  endif()

# player app
project (player VERSION 1.0.0)
  add_executable (${PROJECT_NAME} player/cPlayerApp.h ui/cUI.h
                                  player/playerMain.cpp player/cPlayerUI.cpp player/cPlayerApp.cpp ui/cUI.cpp)

  target_include_directories (${PROJECT_NAME} PRIVATE app common imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoders dvb net imgui
                                                   opengl32 d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoders dvb net imgui
                                                   asound stdc++fs pthread)
  endif()

# demo imgui app
project (demo VERSION 1.0.0)
  file (GLOB HEADER demo/*.h ui/*.h imgui/*.h common/*.h app/*.h)
  file (GLOB SOURCE demo/*.cpp ui/cUI.cpp)
  add_executable (demo ${SOURCE} ${HEADER})

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
  #target_include_directories (${PROJECT_NAME} PRIVATE common fmt fmt/include stb imgui implot)
  target_include_directories (${PROJECT_NAME} PRIVATE app common stb imgui implot)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE app imgui implot opengl32 d3d11 d3dcompiler)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE app imgui implot GL)
  endif()

# telly app
project (telly VERSION 1.0.0)
  add_executable (${PROJECT_NAME} telly/cTellyApp.h ui/cUI.h
                                  telly/cTellyApp.cpp telly/cTellyUI.cpp telly/tellyMain.cpp ui/cUI.cpp)

  target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
  target_include_directories (${PROJECT_NAME} PRIVATE app common implot imgui stb)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio decoders imgui implot
                                                   libmfx opengl32 dxgi d3d11 d3dcompiler ws2_32)
  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio decoders imgui implot
                                                   asound stdc++fs pthread)
  endif()

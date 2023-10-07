#cmake .. -DBUILD_GRAPHICS=GLES_3_1
#
cmake_minimum_required (VERSION 3.18)
cmake_policy (SET CMP0072 NEW)
#
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
#
# options - GL_2_1, GL_3, GLES_3_0, GLES_3_1, GLES_3_2, VULKAN, DX_11 SDL_GL_3 SDL_GLES_3
#
if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  set (BUILD_GRAPHICS "GL_3" CACHE STRING "build graphics option")
  option (BUILD_MFX "Build mfx lib" ON)
  option (BUILD_CAPTURE "Build capture console app" ON)
elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
  set (BUILD_GRAPHICS "GLES_3_1" CACHE STRING "build graphics option")
  option (BUILD_MFX "Build mfx lib" OFF)
endif()
#
option (BUILD_IMPLOT "Build implot lib" ON)
option (BUILD_DEMO "Build demo app" ON)
option (BUILD_FED "Build fed app" ON)
option (BUILD_TELLY "Build telly app" ON)
option (BUILD_PLAYER "Build player app" ON)
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
message (STATUS "using ${BUILD_GRAPHICS} graphics")
#

# glfw lib
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

# implot lib
project (implot VERSION 1.0.0)
  file (GLOB HEADER_FILES implot/*.h)
  file (GLOB SOURCE_FILES implot/*.cpp)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
  target_include_directories (${PROJECT_NAME} PUBLIC implot imgui)

# libmfx
if (BUILD_MFX AND CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  project (libmfx VERSION 1.0.0)
    file (GLOB HEADER_FILES libmfx/include/*.h libmfx/mfx_dispatch/windows/include/*.h common/*.h)
    file (GLOB SOURCE_FILES libmfx/mfx_dispatch/windows/src/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_MFX)
    target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include libmfx/mfx_dispatch/windows/include)
endif()

# dvb lib
if (BUILD_TELLY OR BUILD_PLAYER)
  project (dvb VERSION 1.0.0)
    file (GLOB HEADER_FILES dvb/*.h utils/*.h)
    file (GLOB SOURCE_FILES dvb/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    if (BUILD_MFX)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_MFX)
    endif()

    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      target_include_directories (${PROJECT_NAME} PRIVATE ffmpeg/inc)
      target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include)
      target_link_directories (${PROJECT_NAME} PRIVATE ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC
                             common libmfx avdevice avfilter avformat avcodec avutil swresample swscale)
      target_link_libraries (${PROJECT_NAME} PRIVATE d3d11 dxgi d3d9 dxva2)
    elseif (BUILD_MFX)
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (LIBMFX REQUIRED IMPORTED_TARGET libmfx)
      pkg_check_modules (LIBVA REQUIRED IMPORTED_TARGET libva)
      pkg_check_modules (LIBVADRM REQUIRED IMPORTED_TARGET libva-drm)
      pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
      target_link_libraries (${PROJECT_NAME} PUBLIC common PkgConfig::FFMPEG)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::LIBMFX PkgConfig::LIBVA)
      target_link_libraries (${PROJECT_NAME} PRIVATE common PkgConfig::LIBMFX PkgConfig::LIBVA PkgConfig::LIBVADRM)
    else ()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
      target_link_libraries (${PROJECT_NAME} PUBLIC common PkgConfig::FFMPEG)
    endif()
endif()

# decoder lib
if (BUILD_PLAYER OR BUILD_CAPTURE)
  project (decoder VERSION 1.0.0)
    file (GLOB HEADER_FILES decoder/*.h common/*.h)
    file (GLOB SOURCE_FILES decoder/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      target_include_directories (${PROJECT_NAME} PUBLIC ffmpeg/inc)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC common avdevice avfilter avformat avcodec avutil swresample swscale)
    elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
      target_link_libraries (${PROJECT_NAME} PUBLIC common PkgConfig::FFMPEG)
    endif()
endif()

if (BUILD_PLAYER)
  # kiss lib
  project (kiss VERSION 1.0.0)
    file (GLOB HEADER_FILES kiss/*.h common/*.h)
    file (GLOB SOURCE_FILES kiss/*.c)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE common)
endif()

if (BUILD_PLAYER)
  # song lib
  project (song VERSION 1.0.0)
    file (GLOB HEADER_FILES song/*.h common/*.h)
    file (GLOB SOURCE_FILES song/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE common decoder kiss)
endif()

if (BUILD_PLAYER)
  # net lib
  project (net VERSION 1.0.0)
    file (GLOB HEADER_FILES net/*.h common/*.h)
    file (GLOB SOURCE_FILES net/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE common)
endif()

# audio lib
if (BUILD_PLAYER OR BUILD_TELLY)
  project (audio VERSION 1.0.0)
    file (GLOB HEADER_FILES audio/*.h decoder/*.h common/*.h)
    file (GLOB SOURCE_FILES audio/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE common)
endif()

# windows audio capture app
if (BUILD_CAPTURE AND CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  ## WIN32 audio wsapi capture main project
  project (captureAudio VERSION 1.0.0)
    file (GLOB HEADER_FILES audio/*.h common/*.h)
    file (GLOB SOURCE_FILES captureAudio/capture.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE decoder winmm avrt audio)
endif()

# demo app
if (BUILD_DEMO)
  project (demo VERSION 1.0.0)
    file (GLOB HEADER_FILES demo/*.h ui/*.h imgui/*.h common/*.h app/*.h)
    file (GLOB SOURCE_FILES demo/*.cpp ui/cUI.cpp)
    add_executable (demo ${SOURCE_FILES} ${HEADER_FILES})

    if (GL2)
      target_compile_definitions (${PROJECT_NAME} PRIVATE GL2)
    endif()

    if (BUILD_IMPLOT)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
      target_link_libraries (${PROJECT_NAME} PRIVATE imgui implot)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE imgui)
    endif()

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler)
    elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      target_link_libraries (${PROJECT_NAME} PRIVATE GL)
    endif()
endif()

# fed app
if (BUILD_FED)
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
endif()

# player app
if (BUILD_PLAYER)
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
endif()

# telly app
if (BUILD_TELLY)
  project (telly VERSION 1.0.0)
    file (GLOB HEADER_FILES telly/*.h ui/*.h imgui/*.h implot/implot.h common/*.h app/*.h)
    file (GLOB SOURCE_FILES telly/tellyMain.cpp telly/cTellyApp.cpp telly/cTellyUI.cpp ui/cUI.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

    if (BUILD_IMPLOT)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
      target_link_libraries (${PROJECT_NAME} PRIVATE implot)
    endif()

    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio imgui)
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      target_link_directories (${PROJECT_NAME} PRIVATE ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PRIVATE libmfx opengl32 dxgi d3d11 d3dcompiler ws2_32)
    elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
    endif()
endif()

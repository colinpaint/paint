cmake_minimum_required (VERSION 3.16)
cmake_policy (SET CMP0072 NEW)

option (BUILD_FED "Build fed" ON)
option (BUILD_TELLY "Build telly" ON)
option (BUILD_PLAYER "Build player" ON)
option (BUILD_CAPTURE "Build capture" ON)
option (BUILD_PAINTBOX "Build paintbox" ON)
option (BUILD_IMPLOT "Build implot" ON)
option (BUILD_FILESYSTEM "Build filesystem" ON)
option (BUILD_MFXTEST "Build mfxtest" ON)

set (OPENGL_VERSION "OPENGL_45")
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

project (utils)
  file (GLOB HEADER_FILES utils/*.h)
  file (GLOB SOURCE_FILES utils/*.cpp utils/*.c)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  if (MSVC)
    target_compile_options (${PROJECT_NAME} PUBLIC /W4 /arch:AVX /fp:fast /std:c++17)
  else()
    target_compile_definitions (${PROJECT_NAME} PUBLIC _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64)
    target_compile_options (${PROJECT_NAME} PUBLIC -Wall -Wextra
                                                   -Wno-missing-field-initializers
                                                   -Wno-format-security -Wno-format-overflow
                                                   -pedantic
                                                   -march=native -flax-vector-conversions -ftree-vectorize
                                                   -g -Ofast
                                                   -std=c++17 )
endif()

# gui
if (BUILD_FED OR BUILD_PAINTBOX OR BUILD_PLAYER OR BUILD_TELLY)
  ### glfw
  set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
  #set (GLFW_USE_WAYLAND ON CACHE BOOL "" FORCE)
  add_subdirectory (glfw)

  # glad
  if (OPENGL_VERSION STREQUAL "OPENGL_21")
    add_subdirectory (glad2.1+fbo+vao)
  elseif (OPENGL_VERSION STREQUAL "OPENGL_45")
    add_subdirectory (glad4.5core)

  # imgui lib
  project (imgui)
    if (MSVC)
      file (GLOB HEADER_FILES imgui/*.h imgui/backends/*.h)
      file (GLOB SOURCE_FILES imgui/*.cpp imgui/backends/*.cpp)
    else()
      file (GLOB HEADER_FILES imgui/*.h
                              imgui/backends/imgui_impl_glfw.h imgui/backends/imgui_impl_opengl3.h)
      file (GLOB SOURCE_FILES imgui/*.cpp
                              imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp)
    endif()
    add_library (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PUBLIC imgui glfw/include)
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)

  # implot lib
  if (BUILD_IMPLOT)
    project (implot)
      file (GLOB HEADER_FILES implot/*.h)
      file (GLOB SOURCE_FILES implot/*.cpp)
      add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
      target_include_directories (${PROJECT_NAME} PUBLIC implot imgui utils)
    endif()
  endif()
endif()

# libmfx lib
if (BUILD_TELLY OR BUILD_MFXTEST)
  if (MSVC)
    project (libmfx)
      file (GLOB HEADER_FILES libmfx/include/*.h libmfx/mfx_dispatch/windows/include/*.h)
      file (GLOB SOURCE_FILES libmfx/mfx_dispatch/windows/src/*.cpp)
      add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
      target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include libmfx/mfx_dispatch/windows/include)
  endif()
endif()

# mfxTest app
if (BUILD_MFXTEST)
  project (mfxtest)
    file (GLOB HEADER_FILES mfxtest/*.h utils/*.h)
    if (MSVC)
      file (GLOB SOURCE_FILES mfxtest/main.cpp mfxtest/utils.cpp)
    else()
      file (GLOB SOURCE_FILES mfxtest/main.cpp)
    endif()
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    if (MSVC)
      target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include)
      target_link_libraries (${PROJECT_NAME} PRIVATE libmfx utils d3d9 Dxva2)
    else()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (MFX REQUIRED IMPORTED_TARGET libmfx)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::MFX)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::MFX)
    endif()
endif()

# dvb lib
if (BUILD_TELLY)
  project (dvb)
    file (GLOB HEADER_FILES dvb/*.h)
    file (GLOB SOURCE_FILES dvb/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    if (MSVC)
      target_include_directories (${PROJECT_NAME} PRIVATE ffmpeg/inc)
      target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC
                             utils libmfx avdevice avfilter avformat avcodec avutil swresample swscale)
    else()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (FFmpeg REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::FFmpeg)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::FFmpeg)
      pkg_check_modules (MFX REQUIRED IMPORTED_TARGET libmfx)
      target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::MFX)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::MFX)
    endif()
endif()

# decoder lib
if (BUILD_PLAYER OR BUILD_CAPTURE)
  project (decoder)
    file (GLOB HEADER_FILES decoder/*.h)
    file (GLOB SOURCE_FILES decoder/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    if (MSVC)
      target_include_directories (${PROJECT_NAME} PUBLIC ffmpeg/inc)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC
                             utils avdevice avfilter avformat avcodec avutil swresample swscale)
    else()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (FFmpeg REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::FFmpeg)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::FFmpeg)
    endif()
endif()

# audio lib
if (BUILD_PLAYER OR BUILD_TELLY)
  project (audio)
    file (GLOB HEADER_FILES audio/*.h decoder/*.h)
    file (GLOB SOURCE_FILES audio/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    target_link_libraries (${PROJECT_NAME} PUBLIC utils)
endif()

# telly app
if (BUILD_TELLY)
  project (telly VERSION 1.0.0)
    file (GLOB HEADER_FILES telly/*.h implot/implot.h imgui/*.h ui/*.h utils/*.h
                            platform/*.h graphics/*.h)
    file (GLOB SOURCE_FILES tv/tvMain.cpp tv/cTvApp.cpp tv/cTvUI.cpp
                            ui/cUI.cpp platform/*.cpp graphics/*.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

    if (BUILD_IMPLOT)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
      target_link_libraries (${PROJECT_NAME} PRIVATE implot)
    endif()

    if (MSVC)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PRIVATE libmfx dvb audio imgui utils glfw glad)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio imgui utils glfw glad)
      target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
    endif()
endif()

# player app
if (BUILD_PLAYER)
  project (kiss)
    file (GLOB HEADER_FILES kiss/*.h)
    file (GLOB SOURCE_FILES kiss/*.c)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)

  project (song)
    file (GLOB HEADER_FILES song/*.h)
    file (GLOB SOURCE_FILES song/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PUBLIC utils decoder kiss)

  project (net)
    file (GLOB HEADER_FILES net/*.h)
    file (GLOB SOURCE_FILES net/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)

  project (player VERSION 1.0.0)
    file (GLOB HEADER_FILES player/*.h utils/*.h platform/*.h graphics/*.h imgui/*.h ui/*.h)
    file (GLOB SOURCE_FILES player/playerMain.cpp player/cPlayerUI.cpp player/cPlayerApp.cpp
                            ui/cUI.cpp platform/*.cpp graphics/*.cpp)
    add_executable (player ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui utils glfw glad)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoder dvb kiss net)

    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE GL)
      target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
    endif()
endif()

# fed app
if (BUILD_FED)
  project (fed VERSION 1.0.0)
    file (GLOB HEADER_FILES fed/*.h utils/*.h platform/*.h graphics/*.h imgui/*.h ui/*.h)
    file (GLOB SOURCE_FILES fed/fedMain.cpp fed/cFedApp.cpp fed/cDocument.cpp
                            fed/cEditUI.cpp fed/cTextEdit.cpp fed/cMemEdit.cpp
                            platform/*.cpp graphics/*.cpp ui/cUI.cpp)

    add_executable (fed ${SOURCE_FILES} ${HEADER_FILES})
    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

    target_link_libraries (${PROJECT_NAME} PRIVATE imgui utils glfw glad)
    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE GL stdc++fs)
    endif()
endif()

# capture app
if (BUILD_CAPTURE AND MSVC)
  ## WIN32 audio wsapi capture main project
  project (captureAudio VERSION 1.0.0)
    file (GLOB HEADER_FILES audio/*.h)
    file (GLOB SOURCE_FILES captureAudio/capture.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_link_libraries (${PROJECT_NAME} PRIVATE decoder winmm avrt audio)
endif()

# filesystem app
if (BUILD_FILESYSTEM)
  project (filesystem)
    file (GLOB HEADER_FILES *.h utils/*.h)
    file (GLOB SOURCE_FILES filesystem/main.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_link_libraries (${PROJECT_NAME} PRIVATE utils)
    if (NOT MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE stdc++fs)
    endif()
endif()

# paintbox app
if (BUILD_PAINTBOX)
  project (paintbox VERSION 1.0.0)
    find_package (OpenGL REQUIRED)

    # executable - main + static registered .cpp
    file (GLOB HEADER_FILES paint/*.h utils/*.h platform/*.h graphics/*.h brush/*.h ui/*.h)
    file (GLOB SOURCE_FILES paint/*.cpp platform/*.cpp graphics/*.cpp brush/*.cpp
                            ui/cUI.cpp ui/cCanvasUI.cpp ui/cMenuUI.cpp ui/cLayersUI.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_compile_definitions (${PROJECT_NAME} PRIVATE OPENGL_VERSION)
    target_include_directories (${PROJECT_NAME} PRIVATE glfw/include stb)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui utils glfw glad)

    if (BUILD_IMPLOT)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
      target_link_libraries (${PROJECT_NAME} PRIVATE implot)
    endif()

    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE Opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE GL)
    endif()
endif()

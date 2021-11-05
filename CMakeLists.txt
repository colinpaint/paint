cmake_minimum_required (VERSION 3.16)
cmake_policy (SET CMP0072 NEW)

option (BUILD_FED "Build fed" OFF)
option (BUILD_TELLY "Build telly" ON)
option (BUILD_PLAYER "Build player" OFF)
option (BUILD_FILESYSTEM "Build filesystem" OFF)
option (BUILD_CAPTURE "Build capture" OFF)
option (BUILD_PAINTBOX "Build paintbox" OFF)
option (BUILD_IMPLOT "Build implot" ON)

set (OPENGL_VERSION "OPENGL_45")
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

project (utils)
  file (GLOB UTILS_HEADER_FILES utils/*.h)
  file (GLOB UTILS_SOURCE_FILES utils/*.cpp utils/*.c)
  add_library (${PROJECT_NAME} ${UTILS_HEADER_FILES} ${UTILS_SOURCE_FILES})

  if (MSVC)
    target_compile_options (${PROJECT_NAME} PUBLIC /W4 /arch:AVX /fp:fast /std:c++17)
  else()
    target_compile_definitions (${PROJECT_NAME} PUBLIC _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64)
    target_compile_options (${PROJECT_NAME} PUBLIC -Wall -Wextra
                                                   -Wno-missing-field-initializers
                                                   -Wno-format-security -Wno-format-overflow
                                                   -pedantic
                                                   -march=native -flax-vector-conversions -ftree-vectorize
                                                   -std=c++17
                                                   -g -Ofast
                                                   )
  endif()

# glfw lib
if (BUILD_PAINTBOX OR BUILD_FED OR BUILD_PLAYER OR BUILD_TELLY)
  ### glfw project
  set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory (glfw)
  endif()

# glad lib
if (BUILD_PAINTBOX OR BUILD_FED OR BUILD_PLAYER OR BUILD_TELLY)
  if (OPENGL_VERSION STREQUAL "OPENGL_21")
    add_subdirectory (glad2.1+fbo+vao)
  elseif (OPENGL_VERSION STREQUAL "OPENGL_45")
    add_subdirectory (glad4.5core)
  endif()
endif()

# imgui,implot lib
if (BUILD_PAINTBOX OR BUILD_FED OR BUILD_PLAYER OR BUILD_TELLY)
  project (imgui)
    file (GLOB IMGUI_HEADER_FILES imgui/*.h)
    file (GLOB IMGUI_SOURCE_FILES imgui/*.cpp)
    if (MSVC)
      file (GLOB BACKEND_HEADER_FILES imgui/backends/*.h)
      file (GLOB BACKEND_SOURCE_FILES imgui/backends/*.cpp)
    else()
      file (GLOB BACKEND_HEADER_FILES imgui/backends/imgui_impl_glfw.h imgui/backends/imgui_impl_opengl3.h)
      file (GLOB BACKEND_SOURCE_FILES imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp)
    endif()
    add_library (${PROJECT_NAME} ${IMGUI_SOURCE_FILES} ${IMGUI_HEADER_FILES} ${BACKEND_SOURCE_FILES} ${BACKEND_HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PUBLIC imgui glfw/include)
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)
endif()

# implot lib
if (BUILD_IMPLOT)
  project (implot)
    file (GLOB HEADER_FILES implot/*.h)
    file (GLOB SOURCE_FILES implot/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_include_directories (${PROJECT_NAME} PUBLIC implot imgui utils)
endif()

# dvb lib
if (BUILD_TELLY)
  project (dvb)
    file (GLOB DVB_HEADER_FILES dvb/*.h ffmpeg/*.h)
    file (GLOB DVB_SOURCE_FILES dvb/*.cpp)
    add_library (${PROJECT_NAME} ${DVB_HEADER_FILES} ${DVB_SOURCE_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE ffmpeg/inc)

    if (MSVC)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils avutil avcodec avformat swresample swscale)
    else()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (LIBAV REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec
                                                        libavutil libswresample libswscale)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::LIBAV)
    endif()
endif()

# decoder lib
if (BUILD_PLAYER OR BUILD_CAPTURE)
  project (decoder)
    file (GLOB DECODER_HEADER_FILES decoder/*.h ffmpeg/*.h)
    file (GLOB DECODER_SOURCE_FILES decoder/*.cpp)
    add_library (${PROJECT_NAME} ${DECODER_HEADER_FILES} ${DECODER_SOURCE_FILES})

    target_include_directories (${PROJECT_NAME} PUBLIC ffmpeg/inc)

    if (MSVC)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils avutil avcodec avformat swresample swscale)
    else()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (LIBAV REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec
                                                        libavutil libswresample libswscale)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::LIBAV)
    endif()
endif()

# audio lib
if (BUILD_PLAYER OR BUILD_TELLY)
  project (audio)
    file (GLOB AUDIO_HEADER_FILES audio/*.h decoder/*.h)
    file (GLOB AUDIO_SOURCE_FILES audio/*.cpp)
    add_library (${PROJECT_NAME} ${AUDIO_HEADER_FILES} ${AUDIO_SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)
endif()

# capture app
if (BUILD_CAPTURE AND MSVC)
  ## WIN32 audio wsapi capture main project
  project (captureAudio VERSION 1.0.0)
    file (GLOB CAPTURE_HEADER_FILES audio/*.h)
    file (GLOB CAPTURE_SOURCE_FILES captureAudio/capture.cpp)
    add_executable (${PROJECT_NAME} ${CAPTURE_SOURCE_FILES} ${CAPTURE_HEADER_FILES})
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
      target_link_options (${PROJECT_NAME} PRIVATE -no-pie)
    endif()
endif()

# telly app
if (BUILD_TELLY)
  project (telly VERSION 1.0.0)
    file (GLOB TELLY_HEADER_FILES telly/*.h implot/implot.h imgui/*.h ui/*.h utils/*.h
                                  platform/*.h graphics/*.h)
    file (GLOB TELLY_SOURCE_FILES tv/tvMain.cpp tv/cTvApp.cpp tv/cTvUI.cpp
                                  ui/cUI.cpp platform/*.cpp graphics/*.cpp)
    add_executable (${PROJECT_NAME} ${TELLY_SOURCE_FILES} ${TELLY_HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

    if (BUILD_IMPLOT)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
      target_link_libraries (${PROJECT_NAME} PRIVATE implot)
    endif()

    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio imgui utils glfw glad)
    if (MSVC)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE GL)
      target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
      target_link_options (${PROJECT_NAME} PRIVATE -no-pie)
    endif()
endif()

# player app
if (BUILD_PLAYER)
  project (kiss)
    file (GLOB KISS_HEADER_FILES kiss/*.h)
    file (GLOB KISS_SOURCE_FILES kiss/*.c)
    add_library (${PROJECT_NAME} ${KISS_HEADER_FILES} ${KISS_SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)

  project (song)
    file (GLOB SONG_HEADER_FILES song/*.h)
    file (GLOB SONG_SOURCE_FILES song/*.cpp)
    add_library (${PROJECT_NAME} ${SONG_HEADER_FILES} ${SONG_SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PUBLIC utils decoder kiss)

  project (net)
    file (GLOB NET_HEADER_FILES net/*.h)
    file (GLOB NET_SOURCE_FILES net/*.cpp)
    add_library (${PROJECT_NAME} ${NET_HEADER_FILES} ${NET_SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)

  project (player VERSION 1.0.0)
    file (GLOB PLAYER_HEADER_FILES player/*.h utils/*.h platform/*.h graphics/*.h imgui/*.h ui/*.h)
    file (GLOB PLAYER_SOURCE_FILES player/playerMain.cpp player/cPlayerUI.cpp player/cPlayerApp.cpp
                                   ui/cUI.cpp
                                   platform/*.cpp graphics/*.cpp)

    add_executable (player ${PLAYER_SOURCE_FILES} ${PLAYER_HEADER_FILES})
    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

    target_link_libraries (${PROJECT_NAME} PRIVATE imgui utils glfw glad)
    target_link_libraries (${PROJECT_NAME} PRIVATE song audio decoder dvb kiss net)

    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE GL)
      target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
      target_link_options (${PROJECT_NAME} PRIVATE -no-pie)
    endif()
endif()

# fed app
if (BUILD_FED)
  project (fed VERSION 1.0.0)
    file (GLOB FED_HEADER_FILES fed/*.h utils/*.h platform/*.h graphics/*.h imgui/*.h ui/*.h)
    file (GLOB FED_SOURCE_FILES fed/fedMain.cpp fed/cFedApp.cpp fed/cDocument.cpp
                                fed/cEditUI.cpp fed/cTextEdit.cpp fed/cMemEdit.cpp
                                platform/*.cpp graphics/*.cpp ui/cUI.cpp)

    add_executable (fed ${FED_SOURCE_FILES} ${FED_HEADER_FILES})
    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

    target_link_libraries (${PROJECT_NAME} PRIVATE imgui utils glfw glad)
    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE GL stdc++fs)
      target_link_options (${PROJECT_NAME} PRIVATE -no-pie)
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
      target_link_options (${PROJECT_NAME} PRIVATE -no-pie)
    endif()
endif()

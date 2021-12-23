#cmake .. -DBUILD_GRAPHICS=GLES_3_1
cmake_minimum_required (VERSION 3.18)
cmake_policy (SET CMP0072 NEW)
#
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
#
# valid options - GL_2_1, GL_3, GLES_3_0, GLES_3_1, GLES_3_2, DX_11
set (BUILD_GRAPHICS "GL_3" CACHE STRING "build graphics option")
#set (BUILD_DOCKING "DOCKING" CACHE STRING "Build imgui docking option")
#
option (BUILD_IMPLOT "Build implot lib" ON)
option (BUILD_MFX "Build mfx lib" ON)
#
# imgui apps
option (BUILD_DEMO "Build demo app" ON)
option (BUILD_FED "Build fed app" ON)
option (BUILD_TELLY "Build telly app" ON)
option (BUILD_PLAYER "Build player app" ON)
option (BUILD_PAINTBOX "Build paintbox app" ON)
#
# cmdline apps
option (BUILD_MFXTEST "Build mfxtest cmdline app" ON)
option (BUILD_CAPTURE "Build capture cmdline app" ON)
#
if (MSVC)
  set (BUILD_MSVC_COMPILE_OPTIONS "/arch:AVX" CACHE STRING "msvc compile options")
else()
  set (BUILD_LINUX_COMPILE_OPTIONS "-Ofast" CACHE STRING "linux compile options")
endif()
#
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)

# utils lib
project (utils)
  file (GLOB HEADER_FILES utils/*.h)
  file (GLOB SOURCE_FILES utils/*.cpp utils/*.c)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  if (MSVC)
    target_compile_options (${PROJECT_NAME} PUBLIC /W4 /fp:fast ${BUILD_MSVC_COMPILE_OPTIONS})
  else()
    target_compile_definitions (${PROJECT_NAME} PUBLIC _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64)
    target_compile_options (${PROJECT_NAME} PUBLIC -Wall -Wextra
                                                   -Wno-missing-field-initializers
                                                   -Wno-format-security -Wno-format-overflow
                                                   -pedantic
                                                   -march=native -flax-vector-conversions -ftree-vectorize
                                                   ${BUILD_LINUX_COMPILE_OPTIONS})
  endif()

# gui
if (BUILD_DEMO OR BUILD_FED OR BUILD_PAINTBOX OR BUILD_PLAYER OR BUILD_TELLY)
  # glfw
  set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
  #set (GLFW_USE_WAYLAND ON CACHE BOOL "" FORCE)
  add_subdirectory (glfw)

  # glad
  if (BUILD_GRAPHICS STREQUAL "GL_2_1")
    add_subdirectory (glad2.1+fbo+vao)
  elseif (BUILD_GRAPHICS STREQUAL "GL_3")
    add_subdirectory (glad4.5core)
  endif()

  # imgui backend lib
  project (backend)
    if (BUILD_GRAPHICS STREQUAL "DX_11")
      file (GLOB HEADER_FILES imgui/*.h imgui/backends/imgui_impl_win32.h imgui/backends/imgui_impl_dx11.h app/cApp.h)
      file (GLOB SOURCE_FILES app/cDx11App.cpp imgui/backends/imgui_impl_win32.cpp imgui/backends/imgui_impl_dx11.cpp)
    elseif (BUILD_GRAPHICS STREQUAL "GL_2_1")
      find_package (OpenGL REQUIRED)
      file (GLOB HEADER_FILES imgui/*.h imgui/backends/imgui_impl_glfw.h imgui/backends/imgui_impl_opengl2.h app/cApp.h)
      file (GLOB SOURCE_FILES app/cGlfwApp.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl2.cpp)
    else() # GL_3, GLES_x_x
      find_package (OpenGL REQUIRED)
      file (GLOB HEADER_FILES imgui/*.h imgui/backends/imgui_impl_glfw.h imgui/backends/imgui_impl_GL3.h app/cApp.h)
      file (GLOB SOURCE_FILES app/cGlfwApp.cpp imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl3.cpp)
    endif()
    add_library (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_compile_definitions (${PROJECT_NAME} PRIVATE ${BUILD_GRAPHICS} ${BUILD_DOCKING})
    target_include_directories (${PROJECT_NAME} PRIVATE glfw/include)
    target_include_directories (${PROJECT_NAME} PUBLIC utils imgui)
    target_link_libraries (${PROJECT_NAME} PUBLIC utils)

    if (BUILD_GRAPHICS STREQUAL "GL_2_1" OR
        BUILD_GRAPHICS STREQUAL "GL_3")
      if (MSVC)
        target_link_libraries (${PROJECT_NAME} PRIVATE glfw glad)
      else()
        target_link_libraries (${PROJECT_NAME} PRIVATE glfw glad GL)
      endif()
    elseif (BUILD_GRAPHICS STREQUAL "GLES_3_0" OR
            BUILD_GRAPHICS STREQUAL "GLES_3_1" OR
            BUILD_GRAPHICS STREQUAL "GLES_3_2")
      target_compile_definitions (${PROJECT_NAME} PRIVATE GLFW_INCLUDE_ES3=1
                                                          IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<GLFW/glfw3.h>
                                                          IMGUI_IMPL_OPENGL_ES3=1)
      target_link_libraries (${PROJECT_NAME} PRIVATE glfw GLESv2)
    endif()

  # imgui lib
  project (imgui)
    file (GLOB HEADER_FILES imgui/*.h)
    file (GLOB SOURCE_FILES imgui/*.cpp)
    add_library (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE utils backend)
    target_link_libraries (${PROJECT_NAME} PUBLIC utils backend)

  # implot lib
  if (BUILD_IMPLOT)
    project (implot)
      file (GLOB HEADER_FILES implot/*.h)
      file (GLOB SOURCE_FILES implot/*.cpp)
      add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
      target_include_directories (${PROJECT_NAME} PUBLIC implot imgui)
  endif()
endif()

# libmfx
if (BUILD_MFX AND MSVC)
  project (libmfx)
    file (GLOB HEADER_FILES libmfx/include/*.h libmfx/mfx_dispatch/windows/include/*.h utils/*.h)
    file (GLOB SOURCE_FILES libmfx/mfx_dispatch/windows/src/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_MFX)
    target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include libmfx/mfx_dispatch/windows/include)
endif()

# mfxTest app
if (BUILD_MFXTEST)
  project (mfxtest)
    file (GLOB HEADER_FILES mfxtest/*.h utils/*.h app/*.h)
    file (GLOB SOURCE_FILES mfxtest/main.cpp mfxtest/utils.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_MFX)
    if (MSVC)
      # use project libmfx
      target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include)
      target_link_libraries (${PROJECT_NAME} PRIVATE libmfx utils d3d11 dxgi d3d9 dxva2)
    else()
      # use installed libmfx,libv,libva-drm
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (LIBMFX REQUIRED IMPORTED_TARGET libmfx)
      pkg_check_modules (LIBVA REQUIRED IMPORTED_TARGET libva)
      pkg_check_modules (LIBVADRM REQUIRED IMPORTED_TARGET libva-drm)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::LIBMFX PkgConfig::LIBVA)
      target_link_libraries (${PROJECT_NAME} PRIVATE utils PkgConfig::LIBMFX PkgConfig::LIBVA PkgConfig::LIBVADRM)
    endif()
endif()

# dvb lib
if (BUILD_TELLY)
  project (dvb)
    file (GLOB HEADER_FILES dvb/*.h utils/*.h)
    file (GLOB SOURCE_FILES dvb/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    if (BUILD_MFX)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_MFX)
    endif()

    if (MSVC)
      target_include_directories (${PROJECT_NAME} PRIVATE ffmpeg/inc)
      target_include_directories (${PROJECT_NAME} PRIVATE libmfx/include)
      target_link_directories (${PROJECT_NAME} PRIVATE ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC
                             utils libmfx avdevice avfilter avformat avcodec avutil swresample swscale)
      target_link_libraries (${PROJECT_NAME} PRIVATE d3d11 dxgi d3d9 dxva2)
    elseif (BUILD_MFX)
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (LIBMFX REQUIRED IMPORTED_TARGET libmfx)
      pkg_check_modules (LIBVA REQUIRED IMPORTED_TARGET libva)
      pkg_check_modules (LIBVADRM REQUIRED IMPORTED_TARGET libva-drm)
      pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::FFMPEG)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::LIBMFX PkgConfig::LIBVA)
      target_link_libraries (${PROJECT_NAME} PRIVATE utils PkgConfig::LIBMFX PkgConfig::LIBVA PkgConfig::LIBVADRM)
    else ()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::FFMPEG)
    endif()
endif()

# decoder lib
if (BUILD_PLAYER OR BUILD_CAPTURE)
  project (decoder)
    file (GLOB HEADER_FILES decoder/*.h utils/*.h)
    file (GLOB SOURCE_FILES decoder/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    if (MSVC)
      target_include_directories (${PROJECT_NAME} PUBLIC ffmpeg/inc)
      target_link_directories (${PROJECT_NAME} PUBLIC ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils avdevice avfilter avformat avcodec avutil swresample swscale)
    else()
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                         libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
      target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
      target_link_libraries (${PROJECT_NAME} PUBLIC utils PkgConfig::FFMPEG)
    endif()
endif()

# audio lib
if (BUILD_PLAYER OR BUILD_TELLY)
  project (audio)
    file (GLOB HEADER_FILES audio/*.h decoder/*.h utils/*.h)
    file (GLOB SOURCE_FILES audio/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

    target_link_libraries (${PROJECT_NAME} PRIVATE utils)
endif()

# capture app
if (BUILD_CAPTURE AND MSVC)
  ## WIN32 audio wsapi capture main project
  project (captureAudio VERSION 1.0.0)
    file (GLOB HEADER_FILES audio/*.h utils/*.h)
    file (GLOB SOURCE_FILES captureAudio/capture.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_link_libraries (${PROJECT_NAME} PRIVATE decoder winmm avrt audio)
endif()

# demo app
if (BUILD_DEMO)
  project (demo VERSION 1.0.0)
    file (GLOB HEADER_FILES demo/*.h ui/*.h imgui/*.h utils/*.h app/*.h)
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
    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE GL)
    endif()
endif()

# fed app
if (BUILD_FED)
  project (fed VERSION 1.0.0)
    file (GLOB HEADER_FILES fed/*.h ui/*.h imgui/*.h utils/*.h app/*.h)
    file (GLOB SOURCE_FILES fed/*.cpp ui/cUI.cpp)
    add_executable (fed ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)
    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE imgui opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE imgui stdc++fs)
    endif()
endif()

# telly app
if (BUILD_TELLY)
  project (telly VERSION 1.0.0)
    file (GLOB HEADER_FILES telly/*.h ui/*.h imgui/*.h implot/implot.h utils/*.h app/*.h)
    file (GLOB SOURCE_FILES telly/tellyMain.cpp telly/cTellyApp.cpp telly/cTellyUI.cpp ui/cUI.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)

    if (BUILD_IMPLOT)
      target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_IMPLOT)
      target_link_libraries (${PROJECT_NAME} PRIVATE implot)
    endif()

    target_link_libraries (${PROJECT_NAME} PRIVATE dvb audio imgui)
    if (MSVC)
      target_link_directories (${PROJECT_NAME} PRIVATE ffmpeg/x64)
      target_link_libraries (${PROJECT_NAME} PRIVATE libmfx opengl32 dxgi d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
    endif()
endif()

# player
if (BUILD_PLAYER)
  # kiss lib
  project (kiss)
    file (GLOB HEADER_FILES kiss/*.h utils/*.h)
    file (GLOB SOURCE_FILES kiss/*.c)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE utils)

  # song lib
  project (song)
    file (GLOB HEADER_FILES song/*.h utils/*.h)
    file (GLOB SOURCE_FILES song/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE utils decoder kiss)

  # net lib
  project (net)
    file (GLOB HEADER_FILES net/*.h utils/*.h)
    file (GLOB SOURCE_FILES net/*.cpp)
    add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
    target_link_libraries (${PROJECT_NAME} PRIVATE utils)

  # player app
  project (player VERSION 1.0.0)
    file (GLOB HEADER_FILES player/*.h ui/*.h imgui/*.h utils/*.h app/*.h)
    file (GLOB SOURCE_FILES player/playerMain.cpp player/cPlayerUI.cpp player/cPlayerApp.cpp ui/cUI.cpp)
    add_executable (player ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE imgui stb)
    target_link_libraries (${PROJECT_NAME} PRIVATE imgui song audio decoder dvb kiss net)
    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE pthread asound stdc++fs)
    endif()
endif()

# paintbox app
if (BUILD_PAINTBOX)
  project (paintbox VERSION 1.0.0)
    # executable - main + static registered .cpp
    file (GLOB HEADER_FILES paint/*.h brush/*.h ui/*.h imgui/*.h utils/*.h app/*.h)
    file (GLOB SOURCE_FILES paint/*.cpp brush/*.cpp ui/cUI.cpp ui/cPaintUI.cpp ui/cMenuUI.cpp ui/cLayersUI.cpp)
    add_executable (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

    target_include_directories (${PROJECT_NAME} PRIVATE stb)
    if (MSVC)
      target_link_libraries (${PROJECT_NAME} PRIVATE imgui Opengl32 d3d11 d3dcompiler ws2_32)
    else()
      target_link_libraries (${PROJECT_NAME} PRIVATE imgui)
    endif()
endif()

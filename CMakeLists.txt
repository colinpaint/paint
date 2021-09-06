# cmake config
cmake_minimum_required (VERSION 3.16)
cmake_policy (SET CMP0072 NEW)

#set (USE_IMPLOT)
set (OPENGL_VERSION "OPENGL_45")
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)
#target_compile_features (project PRIVATE cxx_std_20)

### glfw project
set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory (glfw)

### utils project
project (utils)
  file (GLOB UTILS_HEADER_FILES utils/*.h)
  file (GLOB UTILS_SOURCE_FILES utils/*.cpp utils/*.c)
  add_library (utils ${UTILS_HEADER_FILES} ${UTILS_SOURCE_FILES})

  if (MSVC)
    target_compile_options (utils PUBLIC /W4 /arch:AVX /fp:fast /std:c++20)
  else()
    target_compile_options (utils PUBLIC -Wall -Wextra -Wno-missing-field-initializers -Wno-format-overflow
                                         -flax-vector-conversions -pedantic -Ofast -std=c++2a)
  endif()

project (decoder)
  file (GLOB DECODER_HEADER_FILES decoder/*.h ffmpeg/*.h)
  file (GLOB DECODER_SOURCE_FILES decoder/*.cpp)
  add_library (decoder ${DECODER_HEADER_FILES} ${DECODER_SOURCE_FILES})
  target_include_directories (decoder PUBLIC ffmpeg/inc)
  target_link_directories (decoder PUBLIC ffmpeg/x64)
  target_link_libraries (decoder PUBLIC utils avutil avcodec avformat swresample swscale)

project (audio)
  file (GLOB AUDIO_HEADER_FILES audio/*.h decoder/*.h)
  file (GLOB AUDIO_SOURCE_FILES audio/*.cpp)
  add_library (audio ${AUDIO_HEADER_FILES} ${AUDIO_SOURCE_FILES})
 target_link_libraries (audio PUBLIC utils decoder)

project (kiss)
  file (GLOB KISS_HEADER_FILES kiss/*.h)
  file (GLOB KISS_SOURCE_FILES kiss/*.c)
  add_library (kiss ${KISS_HEADER_FILES} ${KISS_SOURCE_FILES})
  target_link_libraries (kiss PUBLIC utils)

project (song)
  file (GLOB SONG_HEADER_FILES song/*.h)
  file (GLOB SONG_SOURCE_FILES song/*.cpp)
  add_library (song ${SONG_HEADER_FILES} ${SONG_SOURCE_FILES})
  target_link_libraries (song PUBLIC utils)

project (net)
  file (GLOB NET_HEADER_FILES net/*.h)
  file (GLOB NET_SOURCE_FILES net/*.cpp)
  add_library (net ${NET_HEADER_FILES} ${NET_SOURCE_FILES})
  target_link_libraries (net PUBLIC utils)

project (dvb)
  file (GLOB DVB_HEADER_FILES dvb/*.h)
  file (GLOB DVB_SOURCE_FILES dvb/*.cpp)
  add_library (dvb ${DVB_HEADER_FILES} ${DVB_SOURCE_FILES})
  target_link_libraries (dvb PUBLIC utils)

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
  add_library (imgui ${IMGUI_SOURCE_FILES} ${IMGUI_HEADER_FILES} ${BACKEND_SOURCE_FILES} ${BACKEND_HEADER_FILES})
  target_include_directories (imgui PUBLIC imgui glfw/include)
  target_link_libraries (imgui PUBLIC utils)

if (USE_IMPLOT)
  project (implot)
    file (GLOB HEADER_FILES *.h)
    file (GLOB SOURCE_FILES *.cpp)
    add_library (implot ${HEADER_FILES} ${SOURCE_FILES})
    target_include_directories (implot PUBLIC implot imgui utils)
endif()

project (canvas)
  file (GLOB CANVAS_HEADER_FILES canvas/*.h)
  file (GLOB CANVAS_SRCFILES canvas/*.cpp)
  add_library (canvas ${CANVAS_HEADER_FILES} ${CANVAS_SRCFILES})
  target_include_directories (canvas PUBLIC imgui stb)
  target_link_libraries (canvas PUBLIC utils)

### WIN32 audio wsapi capture main project 
if (MSVC)
project (captureAudio VERSION 1.0.0)
  file (GLOB CAPTURE_HEADER_FILES audio/*.h)
  file (GLOB CAPTURE_SOURCE_FILES captureAudio/capture.cpp)
  add_executable (captureAudio ${CAPTURE_SOURCE_FILES} ${CAPTURE_HEADER_FILES})
  target_link_libraries (captureAudio PRIVATE winmm avrt audio)
endif()

### paintbox main project 
project (paintbox VERSION 1.0.0)
  find_package (OpenGL REQUIRED)

  # executable - main + static registered .cpp
  file (GLOB HEADER_FILES platform/*.h graphics/*.h brush/*.h ui/*.h)
  file (GLOB SOURCE_FILES main.cpp platform/*.cpp graphics/*.cpp brush/*.cpp ui/*.cpp)
  add_executable (paintbox ${SOURCE_FILES} ${HEADER_FILES})

  target_compile_definitions (paintbox PUBLIC OPENGL_VERSION)
  target_include_directories (paintbox PUBLIC glfw/include stb)

  if (OPENGL_VERSION STREQUAL "OPENGL_21")
    add_subdirectory (glad2.1+fbo+vao)
  elseif (OPENGL_VERSION STREQUAL "OPENGL_45")
    add_subdirectory (glad4.5core)
  endif()

  target_link_libraries (paintbox PUBLIC canvas imgui utils glfw glad)

  if (USE_IMPLOT)
    target_compile_definitions (paintbox PUBLIC USE_IMPLOT)
    target_link_libraries (paintbox PUBLIC implot)
  endif()

  if (MSVC)
    target_link_libraries (paintbox PUBLIC song audio decoder dvb kiss net opengl32 d3d11 d3dcompiler ws2_32)
  else()
    target_link_libraries (paintbox PUBLIC song audio decoder dvb kiss net pthread asound GL)
    target_link_options (paintbox PUBLIC -no-pie)
  endif()

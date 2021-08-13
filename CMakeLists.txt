# cmake config
cmake_minimum_required (VERSION 3.10)
cmake_policy (SET CMP0072 NEW)

# project name
project (paintbox)

# config
set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited configs" FORCE)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

#set (PLATFORM "PLATFORM_GLFW")
#set (GRAPHICS "OPENGL_45")
set (PLATFORM "PLATFORM_WIN32")
set (GRAPHICS "DIRECTX11")

# compiler,linker flags
if (WIN32)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D ${PLATFORM} /D ${GRAPHICS}")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /arch:AVX")
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D ${PLATFORM} -D ${GRAPHICS}")
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
endif()

# graphics versions
if (GRAPHICS STREQUAL "DIRECTX11")
  include_directories (${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/glm ${CMAKE_SOURCE_DIR}/stb)
else()
  # openGL
  if (GRAPHICS STREQUAL "OPENGL_21")
    # openGL 2.1 with fbo vbo extensions
    include_directories (${CMAKE_SOURCE_DIR}/glad-2.1-fbo-vao/include
                         ${CMAKE_SOURCE_DIR}/glfw/include
                         ${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/glm ${CMAKE_SOURCE_DIR}/stb)
    add_subdirectory (glad-2.1-fbo-vao/)
  else()
    # openGL >= 3.0
    include_directories (${CMAKE_SOURCE_DIR}/glad-4.5-core/include
                         ${CMAKE_SOURCE_DIR}/glfw/include
                         ${CMAKE_SOURCE_DIR}/imgui ${CMAKE_SOURCE_DIR}/glm ${CMAKE_SOURCE_DIR}/stb)
    add_subdirectory (glad-4.5-core/)
  endif()

  find_package (OpenGL REQUIRED)
  set (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (ENKITS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set (GLFW_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
  add_subdirectory (glfw/)
endif()

# subdirectories
add_subdirectory (brushes/)
add_subdirectory (ui/)
add_subdirectory (canvas/)
add_subdirectory (tinyfiledialog/)
add_subdirectory (platform/)
add_subdirectory (graphics/)
add_subdirectory (imgui/)
add_subdirectory (log/)

# main source files, add brushes,ui asstatic class register fails
file (GLOB SRCFILES
  ${CMAKE_SOURCE_DIR}/main.cpp
  ${CMAKE_SOURCE_DIR}/brushes/*.cpp
  ${CMAKE_SOURCE_DIR}/ui/*.cpp
  )
file (GLOB HEADER_FILES
  ${CMAKE_SOURCE_DIR}/canvas/*.h
  ${CMAKE_SOURCE_DIR}/tinyfiledialog/*.h
  ${CMAKE_SOURCE_DIR}/platform/*.h
  ${CMAKE_SOURCE_DIR}/graphics/*.h
  ${CMAKE_SOURCE_DIR}/log/*.h
  ${CMAKE_SOURCE_DIR}/brushes/*.h
  ${CMAKE_SOURCE_DIR}/ui/*.h
  )

# main executable
add_executable (paintbox ${SRCFILES} ${HEADER_FILES})

# link, watch out for order dependent static link in linux
if (WIN32)
  if (GRAPHICS STREQUAL "DIRECTX11")
    target_link_libraries (paintbox canvas tinyfiledialog platform graphics imgui log d3d11 d3dcompiler)
  else()
    target_link_libraries (paintbox canvas tinyfiledialog platform graphics imgui log glad glfw opengl32)
  endif()
else()
  target_link_libraries (paintbox canvas tinyfiledialog platform graphics imgui log glad glfw GL)
endif()

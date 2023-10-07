project (app VERSION 1.0.0)
  # GL
  if (BUILD_GRAPHICS STREQUAL "GL_2_1")
    file (GLOB HEADER_FILES ../imgui/*.h
                            ../imgui/backends/imgui_impl_glfw.h ../imgui/backends/imgui_impl_opengl2.h
                            cApp.h app cGL2Graphics.h)
    file (GLOB SOURCE_FILES cGlfwApp.cpp ../imgui/backends/imgui_impl_glfw.cpp ../imgui/backends/imgui_impl_opengl2.cpp)
  elseif (BUILD_GRAPHICS STREQUAL "GL_3")
    file (GLOB HEADER_FILES ../imgui/*.h
                            ../imgui/backends/imgui_impl_glfw.h ../imgui/backends/imgui_impl_GL3.h
                            cApp.h app cGL3Graphics.h)
    file (GLOB SOURCE_FILES cGlfwApp.cpp ../imgui/backends/imgui_impl_glfw.cpp ../imgui/backends/imgui_impl_opengl3.cpp)

  # GLES
  elseif (BUILD_GRAPHICS STREQUAL "GLES_3_0" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_1" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_2")
    file (GLOB HEADER_FILES ../imgui/*.h
                            ../imgui/backends/imgui_impl_glfw.h ../imgui/backends/imgui_impl_GL3.h
                            cApp.h app cGLES3Graphics.h)
    file (GLOB SOURCE_FILES cGlfwApp.cpp ../imgui/backends/imgui_impl_glfw.cpp ../imgui/backends/imgui_impl_opengl3.cpp)

  elseif (BUILD_GRAPHICS STREQUAL "SDL_GL_3")
    file (GLOB HEADER_FILES ../imgui/*.h
                            ../imgui/backends/imgui_impl_sdl.h ../imgui/backends/imgui_impl_GL3.h
                            cApp.h app cGL3Graphics.h)
    file (GLOB SOURCE_FILES cSdlApp.cpp ../imgui/backends/imgui_impl_sdl.cpp ../imgui/backends/imgui_impl_opengl3.cpp)

  # VULKAN
  elseif (BUILD_GRAPHICS STREQUAL "VULKAN")
    if (BUILD_VALIDATION)
      message (STATUS "using vulkan validation")
    endif()
    file (GLOB HEADER_FILES ../imgui/*.h
                            ../imgui/backends/imgui_impl_glfw.h ../imgui/backends/imgui_impl_vulkan.h
                            cApp.h app cVulkanGraphics.h)
    file (GLOB SOURCE_FILES cGlfwApp.cpp ../imgui/backends/imgui_impl_glfw.cpp ../imgui/backends/imgui_impl_vulkan.cpp)

  # unknown
  else()
    message (FATAL_ERROR, "unknown BUILD_GRAPHICS = ${BUILD_GRAPHICS}")
  endif()

  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  target_compile_definitions (${PROJECT_NAME} PRIVATE ${BUILD_GRAPHICS})
  target_include_directories (${PROJECT_NAME} PRIVATE glfw/include
                                              PUBLIC common ../imgui)
  target_link_libraries (${PROJECT_NAME} PUBLIC common)

  # GL
  if (BUILD_GRAPHICS STREQUAL "GL_2_1" OR
      BUILD_GRAPHICS STREQUAL "GL_3")
    find_package (OpenGL REQUIRED)
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      target_link_libraries (${PROJECT_NAME} PRIVATE glfw glad)
    elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      target_link_libraries (${PROJECT_NAME} PRIVATE glfw glad GL)
    endif()

  # GLES
  elseif (BUILD_GRAPHICS STREQUAL "GLES_3_0" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_1" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_2")
    find_package (OpenGL REQUIRED)
    target_compile_definitions (${PROJECT_NAME} PRIVATE GLFW_INCLUDE_ES3=1
                                                        IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<GLFW/glfw3.h>
                                                        IMGUI_IMPL_OPENGL_ES3=1)
    target_link_libraries (${PROJECT_NAME} PRIVATE glfw GLESv2)

  # SDL GL3
  elseif (BUILD_GRAPHICS STREQUAL "SDL_GL_3")
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      find_package (OpenGL REQUIRED)
      target_include_directories (${PROJECT_NAME} PRIVATE SDL/include)
      target_link_directories (${PROJECT_NAME} PUBLIC SDL/lib/x64)
      target_link_libraries (${PROJECT_NAME} PRIVATE SDL2 glad)
    elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      find_package (PkgConfig REQUIRED)
      pkg_check_modules (SDL2 REQUIRED IMPORTED_TARGET sdl2)
      target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::SDL2)
      target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::SDL2 glad GL)
    endif()

  # VULKAN
  elseif (BUILD_GRAPHICS STREQUAL "VULKAN")
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      find_package (Vulkan REQUIRED)
      target_compile_definitions (${PROJECT_NAME} PRIVATE GLFW_INCLUDE_VULKAN VK_PROTOTYPES)
      if (BUILD_VALIDATION)
        target_compile_definitions (${PROJECT_NAME} PRIVATE VALIDATION)
      endif()
      target_link_libraries (${PROJECT_NAME} PRIVATE glfw Vulkan::Vulkan)
    endif()

  endif()

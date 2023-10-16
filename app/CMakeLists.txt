project (app VERSION 1.0.0)
  # GL
  if (BUILD_GRAPHICS STREQUAL "GL_3")
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
  else()
    message (FATAL_ERROR, "unknown BUILD_GRAPHICS = ${BUILD_GRAPHICS}")
  endif()

  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  target_compile_definitions (${PROJECT_NAME} PRIVATE ${BUILD_GRAPHICS})
  target_include_directories (${PROJECT_NAME} PRIVATE glfw/include
                                              PUBLIC common ../imgui)
  target_link_libraries (${PROJECT_NAME} PUBLIC common)

  # GL
  if (BUILD_GRAPHICS STREQUAL "GL_3")
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
  endif()

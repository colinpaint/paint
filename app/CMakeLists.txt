project (app VERSION 1.0.0)
  if (BUILD_GRAPHICS STREQUAL "GL_3")
    # GL
    add_library (${PROJECT_NAME} cApp.h cGlfwApp.cpp cGL3Graphics.h
                                 myImgui.h myImgui.cpp 
                                 ../imgui/imgui.h ../imgui/imgui.cpp
                                 ../imgui/backends/imgui_impl_glfw.h ../imgui/backends/imgui_impl_glfw.cpp
                                 ../imgui/backends/imgui_impl_opengl3.cpp)

  elseif (BUILD_GRAPHICS STREQUAL "GLES_3_0" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_1" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_2")
    # GLES
    add_library (${PROJECT_NAME} cApp.h cGlfwApp.cpp cGLES3Graphics.h
                                 myImgui.h myImgui.cpp 
                                 ../imgui/imgui.h ../imgui/imgui.cpp
                                 ../imgui/backends/imgui_impl_glfw.h ../imgui/backends/imgui_impl_glfw.cpp
                                 ../imgui/backends/imgui_impl_opengl3.cpp)

  else()
    message (FATAL_ERROR, "unknown BUILD_GRAPHICS = ${BUILD_GRAPHICS}")
  endif()

  target_compile_definitions (${PROJECT_NAME} PRIVATE ${BUILD_GRAPHICS})
  target_include_directories (${PROJECT_NAME} PRIVATE glfw/include
                                              PUBLIC common ../imgui)
  target_link_libraries (${PROJECT_NAME} PUBLIC common)

  if (BUILD_GRAPHICS STREQUAL "GL_3")
    # GL
    find_package (OpenGL REQUIRED)
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      target_link_libraries (${PROJECT_NAME} PRIVATE glfw glad)
    elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      target_link_libraries (${PROJECT_NAME} PRIVATE glfw glad GL)
    endif()

  elseif (BUILD_GRAPHICS STREQUAL "GLES_3_0" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_1" OR
          BUILD_GRAPHICS STREQUAL "GLES_3_2")
    # GLES
    find_package (OpenGL REQUIRED)
    target_compile_definitions (${PROJECT_NAME} PRIVATE GLFW_INCLUDE_ES3=1
                                                        IMGUI_IMPL_OPENGL_LOADER_CUSTOM=<GLFW/glfw3.h>
                                                        IMGUI_IMPL_OPENGL_ES3=1)
    target_link_libraries (${PROJECT_NAME} PRIVATE glfw GLESv2)
  endif()

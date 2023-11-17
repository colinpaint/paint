## can't get imgui internal loader to work, use glad for GL
project (app VERSION 1.0.0)
  if (BUILD_GRAPHICS STREQUAL "GL3")
    message (STATUS "building GL3 app with BUILD_GRAPHICS ${BUILD_GRAPHICS}")

    add_library (${PROJECT_NAME} cApp.h cPlatform.h cGraphics.h cGlfwApp.cpp
                                 myImgui.h myImgui.cpp
                                 cGL3Graphics.h)

    target_include_directories (${PROJECT_NAME} PUBLIC ../imgui)

    find_package (OpenGL REQUIRED)
    target_compile_definitions (${PROJECT_NAME} PRIVATE ${BUILD_GRAPHICS})
    target_link_libraries (${PROJECT_NAME} PUBLIC common imgui PRIVATE glfw glad)
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
      target_link_libraries (${PROJECT_NAME} PRIVATE GL)
    endif()

  elseif (BUILD_GRAPHICS STREQUAL "GLES30" OR
          BUILD_GRAPHICS STREQUAL "GLES31" OR
          BUILD_GRAPHICS STREQUAL "GLES32")
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
      message (FATAL_ERROR, "GLES unavailable in Windows BUILD_GRAPHICS = ${BUILD_GRAPHICS}")
    else()
      message (STATUS "building GLES app with BUILD_GRAPHICS ${BUILD_GRAPHICS}")
    endif()

    add_library (${PROJECT_NAME} cApp.h cPlatform.h cGraphics.h cGlfwApp.cpp
                                 myImgui.h myImgui.cpp
                                 cGLES3Graphics.h)

    target_include_directories (${PROJECT_NAME} PUBLIC ../imgui)

    find_package (OpenGL REQUIRED)
    target_compile_definitions (${PROJECT_NAME} PRIVATE ${BUILD_GRAPHICS}
                                                        IMGUI_IMPL_OPENGL_LOADER_CUSTOM=1
                                                        GLFW_INCLUDE_ES3=1
                                                        IMGUI_IMPL_OPENGL_ES3=1)
    target_link_libraries (${PROJECT_NAME} PUBLIC common PRIVATE glfw GLESv2)

  else()
    message (FATAL_ERROR, "app - unknown BUILD_GRAPHICS = ${BUILD_GRAPHICS}")
  endif()

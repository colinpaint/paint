project (platform)

if (WIN32)
  file (GLOB HEADER_FILES cPlatform.h
                          cWin32Platform.h ../imgui/backends/imgui_impl_win32.h
                          cGlfwPlatform.h ../imgui/backends/imgui_impl_glfw.h)
  file (GLOB SRCFILES cPlatform.cpp
                      cWin32Platform.cpp ../imgui/backends/imgui_impl_win32.cpp
                      cGlfwPlatform.cpp ../imgui/backends/imgui_impl_glfw.cpp)
else()
  file (GLOB HEADER_FILES cPlatform.h
                          cGlfwPlatform.h ../imgui/backends/imgui_impl_glfw.h)
  file (GLOB SRCFILES cPlatform.cpp
                      cGlfwPlatform.cpp ../imgui/backends/imgui_impl_glfw.cpp)
endif()

add_library (platform ${SRCFILES} ${HEADER_FILES})

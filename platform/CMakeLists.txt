project (platform)

file (GLOB HEADER_FILES *.h)

if (WIN32)
  file (GLOB SRCFILES 
    cGlfwPlatform.cpp
    ../imgui/backends/imgui_impl_glfw.cpp
    cWin32Platform.cpp
    ../imgui/backends/imgui_impl_win32.cpp
                      )
else()
  file (GLOB SRCFILES 
    cGlfwPlatform.cpp
    ../imgui/backends/imgui_impl_glfw.cpp
                      )
endif()

add_library (platform ${SRCFILES} ${HEADER_FILES})

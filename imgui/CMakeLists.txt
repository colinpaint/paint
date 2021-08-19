project (imgui)

file (GLOB HEADER_FILES *.h)
file (GLOB SRCFILES *.cpp)

if (WIN32)
  add_library (imgui
    ${HEADER_FILES}
    ${SRCFILES}
    backends/imgui_impl_glfw.h
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.h
    backends/imgui_impl_opengl3.cpp
    backends/imgui_impl_win32.h
    backends/imgui_impl_win32.cpp
    backends/imgui_impl_dx11.h
    backends/imgui_impl_dx11.cpp
    )

else()
  add_library (imgui
    ${HEADER_FILES}
    ${SRCFILES}
    backends/imgui_impl_glfw.h
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_opengl3.h
    backends/imgui_impl_opengl3.cpp
    )

endif()

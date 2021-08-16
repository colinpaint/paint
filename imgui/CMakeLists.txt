project (imgui)

file (GLOB HEADER_FILES *.h)
file (GLOB SRCFILES *.cpp)

if (WIN32)
  add_library (imgui  
    ${HEADER_FILES}
    ${SRCFILES}
    backends/imgui_impl_glfw.cpp
    backends/imgui_impl_win32.cpp
    )

else()
  add_library (imgui  
    ${HEADER_FILES} 
    ${SRCFILES}
    backends/imgui_impl_glfw.cpp
    )

endif()

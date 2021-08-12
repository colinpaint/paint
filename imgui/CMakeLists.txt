project (imgui)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)
add_library (imgui ${SRCFILES} backends/imgui_impl_glfw.cpp ${HEADER_FILES} backends/imgui_impl_glfw.h)

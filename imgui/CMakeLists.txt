project (imgui)

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)

if (MSVC)
  file (GLOB BACKEND_HEADER_FILES backends/*.h)
  file (GLOB BACKEND_SOURCE_FILES backends/*.cpp)
else()
  file (GLOB BACKEND_HEADER_FILES backends/imgui_impl_glfw.h backends/imgui_impl_opengl3.h)
  file (GLOB BACKEND_SOURCE_FILES backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp)
endif()

add_library (imgui ${SOURCE_FILES} ${HEADER_FILES} ${BACKEND_SOURCE_FILES} ${BACKEND_HEADER_FILES})

target_include_directories (imgui PUBLIC . ../glfw/include)
target_link_libraries (imgui PUBLIC utils)

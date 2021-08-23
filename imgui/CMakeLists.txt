project (imgui)

file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.h)

if (MSVC)
  file (GLOB BACKEND_SOURCE_FILES backends/*.cpp)
  file (GLOB BACKEND_HEADER_FILES backends/*.h)
else()
  file (GLOB BACKEND_SOURCE_FILES backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp)
  file (GLOB BACKEND_HEADER_FILES backends/imgui_impl_glfw.h backends/imgui_impl_opengl3.h)
endif()

add_library (imgui ${SOURCE_FILES} ${HEADER_FILES} ${BACKEND_SOURCE_FILES} ${BACKEND_HEADER_FILES} )

target_compile_features (imgui PRIVATE cxx_std_17)

if (MSVC)
  target_compile_options (imgui PRIVATE /W4 /arch:AVX /fp:fast /O2)
else()
  target_compile_options (imgui PRIVATE -Wall -Wextra -pedantic -Ofast)
endif()

target_include_directories (imgui PUBLIC . ../glfw/include)

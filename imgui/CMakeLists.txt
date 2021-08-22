project (imgui)

file (GLOB HEADER_FILES *.h implot/*.h)
file (GLOB SOURCE_FILES *.cpp implot/*.cpp)

if (MSVC)
  file (GLOB BACKEND_HEADER_FILES backends/*.h)
  file (GLOB BACKEND_SOURCE_FILES backends/*.cpp)
else()
  file (GLOB BACKEND_HEADER_FILES backends/imgui_impl_glfw.h backends/imgui_impl_opengl3.h)
  file (GLOB BACKEND_SOURCE_FILES backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp)
endif()

add_library (imgui ${BACKEND_SOURCE_FILES} ${SOURCE_FILES} ${HEADER_FILES} ${BACKEND_HEADER_FILES} )

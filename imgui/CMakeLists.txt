project (imgui VERSION 1.0.0)
  file (GLOB HEADER_FILES *.h)
  file (GLOB SOURCE_FILES *.cpp)

  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES}
                               backends/imgui_impl_opengl3_loader.h
                               backends/imgui_impl_opengl3.cpp
                               backends/imgui_impl_glfw.h backends/imgui_impl_glfw.cpp)

  target_include_directories (${PROJECT_NAME} PUBLIC . PRIVATE ../glfw/include)

project (implot VERSION 1.0.0)
  file (GLOB HEADER_FILES *.h)
  file (GLOB SOURCE_FILES *.cpp)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  target_include_directories (${PROJECT_NAME} PUBLIC ../imgui)

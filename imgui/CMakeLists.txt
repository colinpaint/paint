project (imgui VERSION 1.0.0)
  file (GLOB HEADER_FILES *.h myImgui.h)
  file (GLOB SOURCE_FILES *.cpp ../app/myImgui.cpp)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  target_include_directories (${PROJECT_NAME} PRIVATE common app)
  target_link_libraries (${PROJECT_NAME} PUBLIC app)

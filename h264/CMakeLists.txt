project (h264 VERSION 1.0.0)
  file (GLOB HEADER_FILES *.h)
  file (GLOB SOURCE_FILES *.c)

  add_library (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

  target_link_libraries (${PROJECT_NAME} PRIVATE common)

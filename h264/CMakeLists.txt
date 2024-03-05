# h264 JM software reference decoder library
project (h264 VERSION 1.0.0)
  file (GLOB HEADER_FILES *.h)
  file (GLOB SOURCE_FILES *.c)

  add_library (${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

  if (CMAKE_HOST_WIN32)
  else()
    target_compile_options (${PROJECT_NAME} PUBLIC -Wno-unused-variable
                                                   -Wno-unused-but-set-variable
                                                   -Wno-unused-parameter
                                                   -Wno-unused-function
                                                   -Wno-implicit-fallthrough
                                                   -Wno-sign-compare
                                                   )
  endif()

  target_link_libraries (${PROJECT_NAME} PRIVATE common)

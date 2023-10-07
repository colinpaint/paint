project (common VERSION 1.0.0)
  file (GLOB HEADER_FILES *.h)
  file (GLOB SOURCE_FILES *.cpp *.c)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  target_link_libraries (${PROJECT_NAME} PUBLIC fmt)

  message (STATUS "using ${CMAKE_HOST_SYSTEM_NAME}")
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_compile_options (${PROJECT_NAME} PUBLIC /W4
                                                   /fp:fast
                                                   ${BUILD_MSVC_COMPILE_OPTIONS})

  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    target_compile_definitions (${PROJECT_NAME} PUBLIC _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64)
    target_compile_options (${PROJECT_NAME} PUBLIC -Wall
                                                   -Wextra
                                                   -Wno-missing-field-initializers
                                                   -Wno-format-security
                                                   -Wno-format-overflow
                                                   -pedantic
                                                   -flax-vector-conversions
                                                   -ftree-vectorize
                                                   -fno-exceptions
                                                   -march=native
                                                   -g
                                                   ${BUILD_LINUX_COMPILE_OPTIONS}
                                                   )
  else()
    message (FATAL_ERROR, "unknown CMAKE_HOST_SYSTEM_NAME = ${CMAKE_HOST_SYSTEM_NAME}")
  endif()

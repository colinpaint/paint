project (common VERSION 1.0.0)
  add_library (${PROJECT_NAME} cPointRectColor.h cVecMat.h
                               date.h fileUtils.h utils.h readerWriterQueue.h
                               cDvbUtils.h cDvbUtils.cpp cDvbUtilsHuff.cpp
                               cFileView.h cFileView.cpp
                               cLog.h cLog.cpp
                               cMiniLog.h cMiniLog.cpp
                               tinyfiledialogs.h tinyfiledialogs.c
                               )

  target_include_directories (${PROJECT_NAME} PUBLIC fmt/include)
  target_link_libraries (${PROJECT_NAME} PUBLIC fmt)

  message (STATUS "using ${CMAKE_HOST_SYSTEM_NAME}")
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_compile_options (${PROJECT_NAME} PUBLIC /W4
                                                   /fp:fast
                                                   ${BUILD_MSVC_COMPILE_OPTIONS}
                                                   )

  else (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
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
  endif()

project (common VERSION 1.0.0)
  add_library (${PROJECT_NAME} basicTypes.h utils.h fileUtils.h iOptions.h
                               cBipBuffer.h readerWriterQueue.h
                               cLog.h cLog.cpp
                               cFileView.h cFileView.cpp
                               tinyfiledialogs.h tinyfiledialogs.c)

  target_link_libraries (${PROJECT_NAME} PUBLIC date fmt)

  if (CMAKE_HOST_WIN32)
    target_compile_options (${PROJECT_NAME} PUBLIC /W3
                                                   /EHsc
                                                   /fp:fast
                                                   ${BUILD_MSVC_COMPILE_OPTIONS})
    if (USE_AVX512)
      message (STATUS "windows built with AVX X2 AVX512")
      target_compile_options (${PROJECT_NAME} PUBLIC /arch:AVX512)
    elseif (USE_AVX2)
      message (STATUS "windows built with AVX AVX2")
      target_compile_options (${PROJECT_NAME} PUBLIC /arch:AVX2)
    else()
      message (STATUS "windows built with AVX")
    endif()

  else()
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
                                                   ${BUILD_LINUX_COMPILE_OPTIONS})
    message (STATUS "linux built with native")
  endif()

project (common)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    add_library (${PROJECT_NAME} basicTypes.h date.h
                                 cSemaphore.h
                                 cLog.h cLog.cpp
                                 fileUtils.h
                                 )
  else()
    add_library (${PROJECT_NAME} basicTypes.h date.h
                                 cSemaphore.h
                                 cLog.h cLog.cpp
                                 )
  endif()

  target_include_directories (${PROJECT_NAME} PUBLIC fmt/include)
  target_link_libraries (${PROJECT_NAME} PUBLIC fmt)

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    # windows - tbb prebuilt library
    message (STATUS "using prebuilt windows X86-64 tbb library lib/windows64/tbb12.lib")
    target_compile_options (${PROJECT_NAME} PUBLIC /W4
                                                   /fp:fast
                                                   ${BUILD_MSVC_COMPILE_OPTIONS}
                                                   )
    if (USE_AVX512)
      message (STATUS "windows built with AVX X2 AVX512")
      target_compile_options (${PROJECT_NAME} PUBLIC /arch:AVX512)
    elseif (USE_AVX2)
      message (STATUS "windows built with AVX AVX2")
      target_compile_options (${PROJECT_NAME} PUBLIC /arch:AVX2)
    else()
      message (STATUS "windows built with AVX")
      target_compile_options (${PROJECT_NAME} PUBLIC /arch:AVX)

    target_link_directories (${PROJECT_NAME} PUBLIC ../lib/windows64)
    target_link_libraries (${PROJECT_NAME} PUBLIC tbb12)
    endif()

  else()
    # linux - tbb prebuilt library
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

    if (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
      message (STATUS "using prebuilt linux ARM tbb library lib/arm64/libtbb.lib")
      target_link_directories (${PROJECT_NAME} PUBLIC ../lib/arm64)
    else()
      message (STATUS "using prebuilt linux X86-64 tbb library lib/linux64/libtbb.lib")
      target_link_directories (${PROJECT_NAME} PUBLIC ../lib/linux64)
    endif()

    target_link_libraries (${PROJECT_NAME} PUBLIC tbb pthread stdc++fs)
  endif()

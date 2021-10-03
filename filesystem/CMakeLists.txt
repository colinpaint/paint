# cmake config
cmake_minimum_required (VERSION 3.16)

# project utils always built
project (utils)
file (GLOB UTILS_HEADER_FILES ../paint/utils/*.h)
file (GLOB UTILS_SOURCE_FILES ../paint/utils/*.cpp ../paint/utils/*.c)
add_library (utils ${UTILS_HEADER_FILES} ${UTILS_SOURCE_FILES})

if (MSVC)
  target_compile_options (utils PUBLIC /W4 /arch:AVX /fp:fast /std:c++20)
else()
  target_compile_options (utils PUBLIC -Wall -Wextra
                                       -Wno-format-security -Wno-format-overflow
                                       -flax-vector-conversions -pedantic -Ofast -std=c++2a)
endif()


project (filesystem)
  file (GLOB HEADER_FILES *.h ../paint/utils/*.h)
  file (GLOB SOURCE_FILES main.cpp)

  add_executable (filesystem ${SOURCE_FILES} ${HEADER_FILES})
  target_link_libraries (filesystem PRIVATE utils)

  if (NOT MSVC)
    target_link_libraries (filesystem PRIVATE stdc++fs)
    target_link_options (filesystem PRIVATE -no-pie)
  endif()

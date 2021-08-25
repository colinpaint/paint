project (utils)

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp *.c)

add_library (utils ${HEADER_FILES} ${SOURCE_FILES})

target_compile_features (utils PUBLIC cxx_std_17)

if (MSVC)
  target_compile_options (utils PUBLIC /W4 /arch:AVX /fp:fast)
else()
  target_compile_options (utils PUBLIC -Wall -Wextra -pedantic -Ofast)
endif()

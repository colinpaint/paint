project (utils)

file (GLOB SRCFILES *.cpp *.c)
file (GLOB HEADER_FILES *.h)

add_library (utils ${SRCFILES} ${HEADER_FILES})

target_compile_features (utils PRIVATE cxx_std_17)

if (MSVC)
  target_compile_options (utils PRIVATE /W4 /arch:AVX /fp:fast)
else()
  target_compile_options (utils PRIVATE -Wall -Wextra -pedantic -Ofast)
endif()

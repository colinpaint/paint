# not used because of static register problem
project (brush)

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)

add_library (brush ${SOURCE_FILES} ${HEADER_FILES})

target_compile_features (brush PUBLIC cxx_std_17)

if (MSVC)
  target_compile_options (brush PUBLIC /W4 /arch:AVX /fp:fast)
else()
  target_compile_options (brush PUBLIC -Wall -Wextra -pedantic -Ofast)
endif()

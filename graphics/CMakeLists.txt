# not used because of static register problem
project (graphics)

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)

add_library (graphics ${SOURCE_FILES} ${HEADER_FILES})

target_compile_features (graphics PUBLIC cxx_std_17)

if (MSVC)
  target_compile_options (graphics PUBLIC /W4 /arch:AVX /fp:fast)
else()
  target_compile_options (graphics PUBLIC -Wall -Wextra -pedantic -Ofast)
endif()

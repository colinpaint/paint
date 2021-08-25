# not used because of static register problem
project (platform)

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)

add_library (platform ${SOURCE_FILES} ${HEADER_FILES})

target_compile_features (platform PUBLIC cxx_std_17)

if (MSVC)
  target_compile_options (platform PUBLIC /W4 /arch:AVX /fp:fast)
else()
  target_compile_options (platform PUBLIC -Wall -Wextra -pedantic -Ofast)
endif()

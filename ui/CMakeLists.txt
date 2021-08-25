# not used because of static register problem
project (ui)

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)

add_library (ui ${SOURCE_FILES} ${HEADER_FILES})

target_compile_features (ui PUBLIC cxx_std_17)

if (MSVC)
  target_compile_options (ui PUBLIC /W4 /arch:AVX /fp:fast)
else()
  target_compile_options (ui PUBLIC -Wall -Wextra -pedantic -Ofast)
endif()

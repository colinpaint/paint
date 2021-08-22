project (implot)

file (GLOB SOURCE_FILES *.cpp)
file (GLOB HEADER_FILES *.h)

add_library (implot ${SOURCE_FILES} ${HEADER_FILES})

target_compile_features (implot PRIVATE cxx_std_17)

if (MSVC)
  target_compile_options (implot PRIVATE /W4 /arch:AVX /fp:fast /O2)
else()
  target_compile_options (implot PRIVATE -Wall -Wextra -pedantic -Ofast)
endif()

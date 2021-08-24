project (implot)

file (GLOB HEADER_FILES *.h)
file (GLOB SOURCE_FILES *.cpp)

add_library (implot ${HEADER_FILES} ${SOURCE_FILES})

target_compile_features (implot PRIVATE cxx_std_17)

if (MSVC)
  target_compile_options (implot PRIVATE /W4 /arch:AVX /fp:fast /O2)
else()
  target_compile_options (implot PRIVATE -Wall -Wextra -pedantic -Ofast)
endif()

target_include_directories (implot PUBLIC . ../imgui)

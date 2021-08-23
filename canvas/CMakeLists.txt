project (canvas)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)

add_library (canvas ${SRCFILES} ${HEADER_FILES})

target_compile_features (canvas PRIVATE cxx_std_17)
if (MSVC)
  target_compile_options (canvas PRIVATE /W4 /WX /arch:AVX /fp:fast /O2)
else()
  target_compile_options (canvas PRIVATE -Wall -Wextra -pedantic -Ofast)
endif()

target_include_directories (canvas PUBLIC ../glfw/include ../imgui ../stb)

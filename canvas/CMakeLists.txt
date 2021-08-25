project (canvas)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)

add_library (canvas ${SRCFILES} ${HEADER_FILES})

target_compile_features (canvas PUBLIC cxx_std_17)
if (MSVC)
  target_compile_options (canvas PUBLIC /W4 /WX /arch:AVX /fp:fast)
else()
  target_compile_options (canvas PUBLIC -Wall -Wextra -pedantic -Ofast)
endif()

target_include_directories (canvas PUBLIC ../imgui ../stb)

project (canvas)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)

add_library (canvas ${SRCFILES} ${HEADER_FILES})
target_compile_features (canvas PRIVATE cxx_std_17)

project (utils)

file (GLOB SRCFILES *.cpp *.c)
file (GLOB HEADER_FILES *.h)

add_library (utils ${SRCFILES} ${HEADER_FILES})
target_compile_features (utils PRIVATE cxx_std_17)

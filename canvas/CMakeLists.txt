project (canvas)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)
add_library (canvas ${SRCFILES} ${HEADER_FILES})

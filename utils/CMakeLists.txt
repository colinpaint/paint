project (utils)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)
add_library (utils ${SRCFILES} ${HEADER_FILES})

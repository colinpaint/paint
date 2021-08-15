project (graphics)

file (GLOB HEADER_FILES *.h)
file (GLOB SRCFILES cGraphics.cpp)

add_library (graphics ${SRCFILES} ${HEADER_FILES})

project (brush)

file (GLOB HEADER_FILES *.h)
file (GLOB SRCFILES cBrushBase.cpp)

add_library (brush ${SRCFILES} ${HEADER_FILES})

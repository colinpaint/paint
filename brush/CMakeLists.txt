project (brush)

file (GLOB SRCFILES *.cpp )
file (GLOB HEADER_FILES *.h )
add_library (brush ${SRCFILES} ${HEADER_FILES})

project (brushes)

file (GLOB SRCFILES *.cpp )
file (GLOB HEADER_FILES *.h )
add_library (brushes ${SRCFILES} ${HEADER_FILES})

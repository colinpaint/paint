project (tinyfiledialog)

file (GLOB SRCFILES *.c)
file (GLOB HEADER_FILES *.h)
add_library (tinyfiledialog ${SRCFILES} ${HEADER_FILES})

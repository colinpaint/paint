project (ui)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)
add_library (ui ${SRCFILES} ${HEADER_FILES})

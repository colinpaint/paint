project (log)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)
add_library (log ${SRCFILES} ${HEADER_FILES})

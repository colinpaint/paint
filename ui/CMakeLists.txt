project (ui)

file (GLOB HEADER_FILES *.h)
file (GLOB SRCFILES cUIBase.cpp)

add_library (ui ${SRCFILES} ${HEADER_FILES})

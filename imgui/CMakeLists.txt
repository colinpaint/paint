project (imgui)

file (GLOB SRCFILES *.cpp)
file (GLOB HEADER_FILES *.h)

# imGui backends added by graphics or platform
add_library (imgui ${SRCFILES} ${HEADER_FILES})

project (net)

add_library (${PROJECT_NAME} cHttp.h cHttp.cpp)

target_include_directories (${PROJECT_NAME} PUBLIC common)
target_link_libraries (${PROJECT_NAME} PRIVATE common)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
  target_link_libraries (${PROJECT_NAME} PRIVATE ws2_32)
endif()

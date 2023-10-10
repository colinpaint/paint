project (net)
  add_library (${PROJECT_NAME} cHttp.h cHttp.cpp)

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries (${PROJECT_NAME} PRIVATE common ws2_32)
  else()
    target_link_libraries (${PROJECT_NAME} PRIVATE common)
  endif()

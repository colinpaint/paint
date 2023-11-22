project (common VERSION 1.0.0)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    add_library (${PROJECT_NAME} basicTypes.h date.h utils.h fileUtils.h cBipBuffer.h readerWriterQueue.h
                                 cDvbUtils.h cDvbUtils.cpp cDvbUtilsHuff.cpp
                                 cLog.h cLog.cpp
                                 cMiniLog.h cMiniLog.cpp
                                 cFileView.h cFileView.cpp
                                 tinyfiledialogs.h tinyfiledialogs.c)
  else()
    add_library (${PROJECT_NAME} basicTypes.h date.h utils.h fileUtils.h cBipBuffer.h readerWriterQueue.h
                                 cDvbUtils.h cDvbUtils.cpp cDvbUtilsHuff.cpp
                                 cLog.h cLog.cpp
                                 cMiniLog.h cMiniLog.cpp
                                 tinyfiledialogs.h tinyfiledialogs.c)
  endif()

  target_include_directories (${PROJECT_NAME} PUBLIC fmt/include)
  target_link_libraries (${PROJECT_NAME} PUBLIC fmt)

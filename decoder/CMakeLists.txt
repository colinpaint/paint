project (decoder VERSION 1.0.0)
  file (GLOB HEADER_FILES *.h)
  file (GLOB SOURCE_FILES *.cpp)
  add_library (${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_include_directories (${PROJECT_NAME} PUBLIC ../ffmpeg/inc)
    target_link_directories (${PROJECT_NAME} PUBLIC ../ffmpeg/x64)
    target_link_libraries (${PROJECT_NAME} PUBLIC common avdevice avfilter avformat avcodec avutil swresample swscale)

  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                       libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
    target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
    target_link_libraries (${PROJECT_NAME} PUBLIC common PkgConfig::FFMPEG)
  endif()

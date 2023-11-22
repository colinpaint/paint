project (decoders VERSION 1.0.0)
  add_library (${PROJECT_NAME} iAudioDecoder.h
                               cAudioParser.h cAudioParser.cpp
                               cFFmpegAudioDecoder.h cFFmpegAudioDecoder.cpp)

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_include_directories (${PROJECT_NAME} PUBLIC ../include/ffmpeg)
    target_link_directories (${PROJECT_NAME} PUBLIC ../lib)
    target_link_libraries (${PROJECT_NAME} PUBLIC avdevice avfilter avformat avcodec avutil
                                                  swresample swscale)

  elseif (CMAKE_HOST_SYSTEM_NAME STREQUAL Linux)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec libavutil
                                                       libswresample libswscale)
    target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
    target_link_libraries (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)

  endif()

  target_link_libraries (${PROJECT_NAME} PUBLIC common)

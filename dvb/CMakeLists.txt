project (dvb VERSION 1.0.0)
  add_library (${PROJECT_NAME} cFrame.h cAudioFrame.h cVideoFrame.h cFFmpegVideoFrame.h
                               cDecoder.h cDecoderQueueItem.h
                               cFFmpegVideoDecoder.h  cFFmpegAudioDecoder.h
                               cDvbSource.h cDvbSource.cpp
                               cTransportStream.h cTransportStream.cpp
                               cPlayer.h cPlayer.cpp
                               cRender.h cRender.cpp
                               cVideoRender.h cVideoRender.cpp
                               cAudioRender.h cAudioRender.cpp
                               cSubtitleFrame.h cSubtitleDecoder.h cSubtitleImage.h
                               cSubtitleRender.h cSubtitleRender.cpp
                               )

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_include_directories (${PROJECT_NAME} PUBLIC ../include/ffmpeg)
    target_link_libraries (${PROJECT_NAME} PUBLIC 
                           avdevice avfilter avformat avcodec avutil swresample swscale)
  else ()
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                       libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
    target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
    target_link_libraries (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
  endif()

  target_link_libraries (${PROJECT_NAME} PUBLIC common)

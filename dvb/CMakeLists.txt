project (dvb VERSION 1.0.0)
  add_library (${PROJECT_NAME} cAudioFrame.h cVideoFrame.h
                               cBipBuffer.h cDvbMultiplex.h
                               cDvbUtils.h cDvbUtils.cpp cDvbUtilsHuff.cpp
                               cDvbSource.h cDvbSource.cpp
                               cDvbStream.h cDvbStream.cpp
                               cRender.h cRender.cpp
                               cVideoRender.h cVideoRender.cpp
                               cAudioRender.h cAudioRender.cpp
                               cSubtitleRender.h cSubtitleRender.cpp
                               )

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_include_directories (${PROJECT_NAME} PUBLIC ../include/ffmpeg)
    #target_link_directories (${PROJECT_NAME} PRIVATE ../ffmpeg/x64)
    target_link_libraries (${PROJECT_NAME} PUBLIC common avdevice avfilter avformat avcodec avutil
                                                         swresample swscale)
    target_link_libraries (${PROJECT_NAME} PRIVATE d3d11 dxgi d3d9 dxva2)

  else ()
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET libavdevice libavfilter libavformat libavcodec libavutil
                                                       libswresample libswscale)

    target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
    target_link_libraries (${PROJECT_NAME} PUBLIC common PkgConfig::FFMPEG)

  endif()

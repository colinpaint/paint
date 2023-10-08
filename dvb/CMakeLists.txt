project (dvb VERSION 1.0.0)

  add_library (${PROJECT_NAME} cAudioFrame.h cVideoFrame.h cBipBuffer.h cDvbMultiplex.h
                               cDvbUtils.h cDvbUtils.cpp
                               cDvbSource.h cDvbSource.cpp
                               cDvbStream.h cDvbStream.cpp
                               cRender.h cRender.cpp
                               cAudioRender.h cAudioRender.cpp
                               cVideoRender.h cVideoRender.cpp
                               cSubtitleRender.h cSubtitleRender.cpp
                               )
  if (BUILD_MFX)
    target_compile_definitions (${PROJECT_NAME} PRIVATE BUILD_MFX)
  endif()

  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    target_include_directories (${PROJECT_NAME} PUBLIC ../include/ffmpeg)
    target_include_directories (${PROJECT_NAME} PRIVATE ../libmfx/include)

    target_link_directories (${PROJECT_NAME} PRIVATE ../ffmpeg/x64)

    target_link_libraries (${PROJECT_NAME} PUBLIC
                           common libmfx avdevice avfilter avformat avcodec avutil swresample swscale)
    target_link_libraries (${PROJECT_NAME} PRIVATE d3d11 dxgi d3d9 dxva2)

  elseif (BUILD_MFX)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (LIBMFX REQUIRED IMPORTED_TARGET libmfx)
    pkg_check_modules (LIBVA REQUIRED IMPORTED_TARGET libva)
    pkg_check_modules (LIBVADRM REQUIRED IMPORTED_TARGET libva-drm)
    pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                       libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)

    target_include_directories (${PROJECT_NAME} PUBLIC PkgConfig::FFMPEG)
    target_link_libraries (${PROJECT_NAME} PUBLIC common PkgConfig::FFMPEG)

    target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::LIBMFX PkgConfig::LIBVA)
    target_link_libraries (${PROJECT_NAME} PRIVATE common PkgConfig::LIBMFX PkgConfig::LIBVA PkgConfig::LIBVADRM)

  else ()
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (FFMPEG REQUIRED IMPORTED_TARGET
                       libavdevice libavfilter libavformat libavcodec libavutil libswresample libswscale)
    target_include_directories (${PROJECT_NAME} PRIVATE PkgConfig::FFMPEG)
    target_link_libraries (${PROJECT_NAME} PUBLIC common PkgConfig::FFMPEG)

  endif()

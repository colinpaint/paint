project (audio VERSION 1.0.0)
  if (CMAKE_HOST_SYSTEM_NAME STREQUAL Windows)
    add_library (${PROJECT_NAME} iAudio.h audioHelpers.h
                                 audioWASAPI.h cCaptureWASAPI.h cCaptureWASAPI.cpp
                                 kiss_fft.h kiss_fft_guts.h kiss_fftr.h
                                 kiss_fftr.c kiss_fft.c
                                 cLinuxAudio.h
                                 #cWinAudio16.h cWinAudio16.cpp
                                 cWinAudio32.h cWinAudio32.cpp)
    target_link_libraries (${PROJECT_NAME} PRIVATE common)

  else()
    add_library (${PROJECT_NAME} iAudio.h
                                 kiss_fft.h kiss_fft_guts.h kiss_fftr.h
                                 kiss_fftr.c kiss_fft.c
                                 cLinuxAudio.h)
    #target_link_libraries (${PROJECT_NAME} PRIVATE common asound)
    target_link_libraries (${PROJECT_NAME} PRIVATE common pulse-simple)
  endif()

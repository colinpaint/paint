project (audio VERSION 1.0.0)
  if (CMAKE_HOST_WIN32)
    add_library (${PROJECT_NAME} iAudio.h
                                 audioHelpers.h audioWASAPI.h
                                 cCaptureWASAPI.h cCaptureWASAPI.cpp
                                 kiss_fft.h kiss_fft_guts.h kiss_fftr.h kiss_fftr.c kiss_fft.c)
  else()
    add_library (${PROJECT_NAME} iAudio.h
                                 cLinuxAudio.h
                                 kiss_fft.h kiss_fft_guts.h kiss_fftr.h kiss_fftr.c kiss_fft.c)
    target_link_libraries (${PROJECT_NAME} PRIVATE pulse-simple)
  endif()

  target_link_libraries (${PROJECT_NAME} PRIVATE common)

project (audio VERSION 1.0.0)
  add_library (${PROJECT_NAME} iAudio.h audioHelpers.h cLinuxAudio.h
                               audioWASAPI.h cCaptureWASAPI.h cCaptureWASAPI.cpp
                               cWinAudio16.h cWinAudio16.cpp
                               cWinAudio32.h cWinAudio32.cpp
                               kiss_fft.h kiss_fft_guts.h kiss_fftr.h
                               kiss_fftr.c kiss_fft.c)

  target_link_libraries (${PROJECT_NAME} PRIVATE common)

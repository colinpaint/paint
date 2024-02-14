project (dvb VERSION 1.0.0)
  add_library (${PROJECT_NAME} cDvbMultiplex.h
                               cDvbSource.h cDvbSource.cpp
                               cTransportStream.h cTransportStream.cpp)

  target_link_libraries (${PROJECT_NAME} PUBLIC decoders common)

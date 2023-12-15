project (song VERSION 1.0.0)
  add_library (${PROJECT_NAME} cSong.h cSong.cpp
                               cSongLoader.h cSongLoader.cpp
                               cSongPlayer.h cSongPlayer.cpp
                               iVideoPool.h cSongVideoPool.cpp)

  target_link_libraries (${PROJECT_NAME} PUBLIC common decoders audio)

// cSongPlayer.h
#pragma once
#include <thread>
class cSong;

class cSongPlayer {
public:
  cSongPlayer (cSong* song, bool streaming);
  ~cSongPlayer() {}

  void exit() { mExit = true; }
  void wait();

private:
  std::thread mPlayerThread;
  bool mExit = false;
  bool mRunning = true;
  };

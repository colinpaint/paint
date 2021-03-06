// cPlayerApp.h - playerApp info holder from playerMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../app/cApp.h"

class cPlatform;
class cGraphics;

class cSong;
class cSongLoader;
//}}}

class cPlayerApp : public cApp {
public:
  cPlayerApp (const cPoint& windowSize, bool fullScreen, bool vsync);
  virtual ~cPlayerApp() = default;

  std::string getSongName() const { return mSongName; }
  cSong* getSong() const;

  bool setSongName (const std::string& songName);
  bool setSongSpec (const std::vector <std::string>& songSpec);

  virtual void drop (const std::vector<std::string>& dropItems) final;

private:
  cSongLoader* mSongLoader;
  std::string mSongName;
  };

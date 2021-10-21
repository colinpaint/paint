// cPlayerApp.h - playerApp info holder from playerMain
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/cApp.h"

class cPlatform;
class cGraphics;

class cSong;
class cSongLoader;
//}}}

class cPlayerApp : public cApp {
public:
  cPlayerApp (cPlatform& platform, cGraphics& graphics, ImFont* mainFont, ImFont* monoFont);
  ~cPlayerApp() = default;

  std::string getSongName() const { return mSongName; }
  cSong* getSong() const;

  bool setSongName (const std::string& songName);
  bool setSongSpec (const std::vector <std::string>& songSpec);

private:
  cSongLoader* mSongLoader;

  std::string mSongName;
  };

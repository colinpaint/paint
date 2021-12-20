// cPlayerApp.cpp - playerApp info holder from playerMain
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS

#include "cPlayerApp.h"

#include <cstdint>
#include <string>
#include <vector>

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/cFileUtils.h"

#include "../song/cSong.h"
#include "../song/cSongLoader.h"

using namespace std;
//}}}

cPlayerApp::cPlayerApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp("player",windowSize, fullScreen, vsync) {
  mSongLoader = new cSongLoader();
  }

cSong* cPlayerApp::getSong() const {
  return mSongLoader->getSong();
  }

bool cPlayerApp::setSongName (const std::string& songName) {

  mSongName = songName;

  // load song
  const vector <string>& strings = { songName };
  mSongLoader->launchLoad (strings);

  return true;
  }

bool cPlayerApp::setSongSpec (const vector <string>& songSpec) {

  mSongName = songSpec[0];
  mSongLoader->launchLoad (songSpec);
  return true;
  }

void cPlayerApp::drop (const vector<string>& dropItems) {

  for (auto& item : dropItems) {
    cLog::log (LOGINFO, item);
    setSongName (cFileUtils::resolve (item));
    }
  }

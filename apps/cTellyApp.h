// cTellyApp.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <thread>

// utils
#include "../common/basicTypes.h"
#include "../common/iOptions.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../app/myImgui.h"

// dvb
#include "../dvb/cDvbMultiplex.h"
#include "../dvb/cDvbSource.h"
#include "../dvb/cTransportStream.h"

// decoders
//{{{  include libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  #include "libavutil/motion_vector.h"
  #include "libavutil/frame.h"
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}
#include "../decoders/cAudioFrame.h"
#include "../decoders/cVideoFrame.h"
#include "../decoders/cSubtitleFrame.h"
#include "../decoders/cSubtitleImage.h"
#include "../decoders/cFFmpegVideoDecoder.h"

// dvb
#include "../dvb/cVideoRender.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cPlayer.h"
//}}}

//{{{
const std::vector <cDvbMultiplex> kDvbMultiplexes = {
    {"hd", 626000000,
      {"BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "Channel 5 HD", "ITV1 HD", "Channel 4 HD"},
      {"bbc1",          "bbc2",       "bbc3",         "bbc4",        "c5",           "itv",     "c4"}
    },

    {"itv", 650000000,
      {"ITV1",   "ITV2",   "ITV3",   "ITV4",   "Channel 4", "More 4",  "Film4" ,  "E4",   "Channel 5"},
      {"itv1sd", "itv2sd", "itv3sd", "itv4sd", "c4sd",      "more4sd", "film4sd", "e4sd", "c5sd"}
    },

    {"bbc", 674000000,
      {"BBC ONE S West", "BBC TWO", "BBC THREE", "BBC FOUR", "BBC NEWS"},
      {"bbc1sd",         "bbc2sd",  "bbc3sd",    "bbc4sd",   "bbcNewsSd"}
    }
  };
//}}}
//{{{
class cTellyOptions : public cApp::cOptions,
                      public cTransportStream::cOptions,
                      public cRender::cOptions,
                      public cAudioRender::cOptions,
                      public cVideoRender::cOptions,
                      public cFFmpegVideoDecoder::cOptions {
public:
  virtual ~cTellyOptions() = default;

  std::string getString() const { return "song sub motion hd|bbc|itv filename"; }

  // vars
  bool mPlaySong = false;
  bool mShowEpg = true;
  bool mShowSubtitle = false;
  bool mShowMotionVectors = false;

  cDvbMultiplex mMultiplex = kDvbMultiplexes[0];
  std::string mFileName;
  };
//}}}

class cTellyApp : public cApp {
public:
  cTellyApp (cTellyOptions* options, iUI* ui) : cApp ("telly", options, ui), mOptions(options) {}
  virtual ~cTellyApp() = default;

  cTellyOptions* getOptions() { return mOptions; }
  bool hasTransportStream() { return mTransportStream != nullptr; }
  cTransportStream& getTransportStream() { return *mTransportStream; }

  // liveDvbSource
  bool hasDvbSource() const { return mDvbSource; }
  cDvbSource& getDvbSource() { return *mDvbSource; }
  void liveDvbSource (const cDvbMultiplex& multiplex, cTellyOptions* options);

  // fileSource
  bool hasFileSource() { return mFileSource; }
  uint64_t getStreamPos() const { return mStreamPos; }
  uint64_t getFilePos() const { return mFilePos; }
  size_t getFileSize() const { return mFileSize; }
  std::string getFileName() const { return mOptions->mFileName; }
  void fileSource (const std::string& filename, cTellyOptions* options);

  // actions
  void skip (int64_t skipPts);

  void togglePlay() { mTransportStream->togglePlay(); }
  void toggleShowEpg() { mOptions->mShowEpg = !mOptions->mShowEpg; }
  void toggleShowSubtitle() { mOptions->mShowSubtitle = !mOptions->mShowSubtitle; }
  void toggleShowMotionVectors() { mOptions->mShowMotionVectors = !mOptions->mShowMotionVectors; }

  void drop (const std::vector<std::string>& dropItems);

private:
  cTellyOptions* mOptions;
  cTransportStream* mTransportStream = nullptr;

  // liveDvbSource
  std::thread mLiveThread;
  cDvbSource* mDvbSource = nullptr;
  cDvbMultiplex mMultiplex;
  std::string mRecordRoot;

  // fileSource
  bool mFileSource = false;
  FILE* mFile = nullptr;
  int64_t mStreamPos = 0;
  int64_t mFilePos = 0;
  size_t mFileSize = 0;
  };

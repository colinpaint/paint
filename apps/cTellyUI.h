// cTellyUI.h
//{{{  includes
#pragma once

//#include <cstdint>
#include <string>
#include <vector>
#include <thread>

#include "../date/include/date/date.h"
#include "../common/basicTypes.h"
#include "../common/iOptions.h"

// app
#include "../app/cApp.h"

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

#include "cTellyApp.h"

using namespace std;

class cMultiView;
//}}}

class cTellyUI : public cApp::iUI {
public:
  cTellyUI();
  virtual ~cTellyUI() = default;
  void draw (cApp& app);

private:
  enum eTab { eTelly, ePids, eRecord };
  inline static const std::vector<std::string> kTabNames = { "telly", "pids", "recorded" };

  void hitShow (eTab tab);

  void drawPidMap (cTransportStream& transportStream);
  void drawRecordedFileNames (cTransportStream& transportStream);

  void keyboard (cTellyApp& tellyApp);
  void hitSpace (cTellyApp& tellyApp);
  void hitControlLeft (cTellyApp& tellyApp);
  void hitControlRight (cTellyApp& tellyApp);
  void hitLeft (cTellyApp& tellyApp);
  void hitRight (cTellyApp& tellyApp);
  void hitUp (cTellyApp& tellyApp);
  void hitDown (cTellyApp& tellyApp);

  // vars
  eTab mTab = eTelly;
  cMultiView& mMultiView;

  // pidInfo tabs
  int64_t mMaxPidPackets = 0;
  size_t mPacketChars = 3;
  size_t mMaxNameChars = 3;
  size_t mMaxSidChars = 3;
  size_t mMaxPgmChars = 3;
  };

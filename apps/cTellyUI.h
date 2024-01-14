// cTellyUI.h
#pragma once
//{{{  includes
#include <cstdint>
#include <string>

#include "../date/include/date/date.h"
#include "../common/basicTypes.h"
#include "../common/iOptions.h"

// app
#include "../app/cApp.h"

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
#include "../dvb/cTransportStream.h"
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
  enum eTab { eNone, eEpg, ePids, eRecord };

  void hitTab (cTellyApp& tellyApp, uint8_t tabIndex);
  void hitSpace (cTellyApp& tellyApp);
  void hitControlLeft (cTellyApp& tellyApp);
  void hitControlRight (cTellyApp& tellyApp);
  void hitLeft (cTellyApp& tellyApp);
  void hitRight (cTellyApp& tellyApp);
  void hitUp (cTellyApp& tellyApp);
  void hitDown (cTellyApp& tellyApp);
  void keyboard (cTellyApp& tellyApp);

  void drawPids (cTransportStream& transportStream);
  void drawRecordedFileNames (cTransportStream& transportStream, ImVec2 pos);

  // vars
  uint8_t mTabIndex = eNone;
  cMultiView& mMultiView;

  // pidInfo tabs
  int64_t mMaxPidPackets = 0;
  size_t mPacketChars = 3;
  size_t mMaxNameChars = 3;
  size_t mMaxSidChars = 3;
  size_t mMaxPgmChars = 3;
  };

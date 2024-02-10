// cPlayerUI.h
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

#include "cPlayerApp.h"

using namespace std;

class cMultiView;
//}}}

class cPlayerUI : public cApp::iUI {
public:
  cPlayerUI();
  virtual ~cPlayerUI() = default;

  void draw (cApp& app);

private:
  void hitTab (cPlayerApp& PlayerApp, uint8_t tabIndex);
  void hitSpace (cPlayerApp& playerApp);
  void hitControlLeft (cPlayerApp& tellyApp);
  void hitControlRight (cPlayerApp& tellyApp);
  void hitLeft (cPlayerApp& tellyApp);
  void hitRight (cPlayerApp& tellyApp);
  void hitUp (cPlayerApp& tellyApp);
  void hitDown (cPlayerApp& tellyApp);
  void keyboard (cPlayerApp& tellyApp);

  // vars
  cMultiView& mMultiView;
  };

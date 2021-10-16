// cPlayerUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImguiWidgets.h"

// ui
#include "../ui/cUI.h"

#include "../platform/cPlatform.h"
#include "../graphics/cGraphics.h"

// decoder
#include "../utils/cFileView.h"

// song
#include "../song/cSongLoader.h"

#include "../player/cDrawSong.h"

// utils
#include "../utils/cLog.h"
#include "../utils/date.h"

using namespace std;
//}}}
//{{{  const
static const vector<string> kRadio1 = {"r1", "a128"};
static const vector<string> kRadio2 = {"r2", "a128"};
static const vector<string> kRadio3 = {"r3", "a320"};
static const vector<string> kRadio4 = {"r4", "a64"};
static const vector<string> kRadio5 = {"r5", "a128"};
static const vector<string> kRadio6 = {"r6", "a128"};

static const vector<string> kBbc1   = {"bbc1", "a128"};
static const vector<string> kBbc2   = {"bbc2", "a128"};
static const vector<string> kBbc4   = {"bbc4", "a128"};
static const vector<string> kNews   = {"news", "a128"};
static const vector<string> kBbcSw  = {"sw", "a128"};

static const vector<string> kWqxr  = {"http://stream.wqxr.org/js-stream.aac"};
static const vector<string> kDvb  = {"dvb"};

static const vector<string> kRtp1  = {"rtp 1"};
static const vector<string> kRtp2  = {"rtp 2"};
static const vector<string> kRtp3  = {"rtp 3"};
static const vector<string> kRtp4  = {"rtp 4"};
static const vector<string> kRtp5  = {"rtp 5"};
//}}}

class cPlayerUI : public cUI {
public:
  //{{{
  cPlayerUI (const string& name) : cUI(name) {
    }
  //}}}
  //{{{
  virtual ~cPlayerUI() {
    // close the file mapping object
    }
  //}}}

  //{{{
  void addToDrawList (cApp& app) final {

    if (!mSongLoaded) {
      //{{{  load song
      const vector <string>& strings = { app.getName() };
      if (!app.getName().empty()) {
        mSongLoader.launchLoad (strings);
        mSongLoaded = true;
        }
      }
      //}}}

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

    ImGui::Begin ("player", &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

    //{{{  draw top buttons
    // monoSpaced buttom
    if (toggleButton ("mono",  mShowMonoSpaced))
      toggleShowMonoSpaced();

    // vsync button,fps
    if (app.getPlatform().hasVsync()) {
      // vsync button
      ImGui::SameLine();
      if (toggleButton ("vSync", app.getPlatform().getVsync()))
        app.getPlatform().toggleVsync();

      // fps text
      ImGui::SameLine();
      ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      }

    // fullScreen button
    if (app.getPlatform().hasFullScreen()) {
      ImGui::SameLine();
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      }

    // vertice debug
    ImGui::SameLine();
    ImGui::Text (fmt::format ("{}:{}",
                 ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
    //}}}
    //{{{  draw radio buttons
    if (ImGui::Button ("radio1"))
      mSongLoader.launchLoad (kRadio1);
    if (ImGui::Button ("radio2"))
      mSongLoader.launchLoad (kRadio2);
    if (ImGui::Button ("radio3"))
      mSongLoader.launchLoad (kRadio3);
    if (ImGui::Button ("radio4"))
      mSongLoader.launchLoad (kRadio4);
    if (ImGui::Button ("radio5"))
      mSongLoader.launchLoad (kRadio5);
    if (ImGui::Button ("radio6"))
      mSongLoader.launchLoad (kRadio6);
    if (ImGui::Button ("wqxr"))
      mSongLoader.launchLoad (kWqxr);
    //}}}

    // draw song
    if (isDrawMonoSpaced())
      ImGui::PushFont (app.getMonoFont());

    mDrawSong.draw (mSongLoader.getSong(), isDrawMonoSpaced());

    if (isDrawMonoSpaced())
      ImGui::PopFont();

    ImGui::End();
    }
  //}}}

private:
  bool isDrawMonoSpaced() { return mShowMonoSpaced; }
  void toggleShowMonoSpaced() { mShowMonoSpaced = ! mShowMonoSpaced; }

  // vars
  bool mOpen = true;
  bool mShowMonoSpaced = false;

  bool mSongLoaded = false;
  cSongLoader mSongLoader;
  cDrawSong mDrawSong;

  //{{{
  static cUI* create (const string& className) {
    return new cPlayerUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("player", &create);
  };

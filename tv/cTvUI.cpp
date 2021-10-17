// cTvUI.cpp
//{{{  includes
#include <cstdint>
#include <array>
#include <vector>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

// ui
#include "../ui/cUI.h"

#include "../platform/cPlatform.h"
#include "../graphics/cGraphics.h"

// utils
#include "../utils/cLog.h"
#include "../utils/date.h"

using namespace std;
//}}}

namespace {
  //{{{  channels const
  const vector<string> kRadio1 = {"r1", "a128"};
  const vector<string> kRadio2 = {"r2", "a128"};
  const vector<string> kRadio3 = {"r3", "a320"};
  const vector<string> kRadio4 = {"r4", "a64"};
  const vector<string> kRadio5 = {"r5", "a128"};
  const vector<string> kRadio6 = {"r6", "a128"};

  const vector<string> kBbc1   = {"bbc1", "a128"};
  const vector<string> kBbc2   = {"bbc2", "a128"};
  const vector<string> kBbc4   = {"bbc4", "a128"};
  const vector<string> kNews   = {"news", "a128"};
  const vector<string> kBbcSw  = {"sw", "a128"};

  const vector<string> kWqxr  = {"http://stream.wqxr.org/js-stream.aac"};
  const vector<string> kDvb  = {"dvb"};

  const vector<string> kRtp1  = {"rtp 1"};
  const vector<string> kRtp2  = {"rtp 2"};
  const vector<string> kRtp3  = {"rtp 3"};
  const vector<string> kRtp4  = {"rtp 4"};
  const vector<string> kRtp5  = {"rtp 5"};
  //}}}
  }

//{{{
class cDrawTv : public cDrawContext {
public:
  cDrawTv() : cDrawContext (kPalette) {}
  //{{{
  void draw (bool monoSpaced) {

    update (24.f, monoSpaced);
    layout();
    }
  //}}}

private:
  //{{{  palette const
  inline static const uint8_t eBackground =  0;
  inline static const uint8_t eText =        1;
  inline static const uint8_t eFreq =        2;

  inline static const uint8_t ePeak =        3;
  inline static const uint8_t ePowerPlayed = 4;
  inline static const uint8_t ePowerPlay =   5;
  inline static const uint8_t ePowerToPlay = 6;

  inline static const uint8_t eRange       = 7;
  inline static const uint8_t eOverview    = 8;

  inline static const uint8_t eLensOutline = 9;
  inline static const uint8_t eLensPower  = 10;
  inline static const uint8_t eLensPlay   = 11;

  inline static const vector <ImU32> kPalette = {
  //  aabbggrr
    0xff000000, // eBackground
    0xffffffff, // eText
    0xff00ffff, // eFreq
    0xff606060, // ePeak
    0xffc00000, // ePowerPlayed
    0xffffffff, // ePowerPlay
    0xff808080, // ePowerToPlay
    0xff800080, // eRange
    0xffa0a0a0, // eOverView
    0xffa000a0, // eLensOutline
    0xffa040a0, // eLensPower
    0xffa04060, // eLensPlay
    };
  //}}}
  //{{{
  void layout () {

    // check for window size change, refresh any caches dependent on size
    ImVec2 size = ImGui::GetWindowSize();
    mChanged |= (size.x != mSize.x) || (size.y != mSize.y);
    mSize = size;
    }
  //}}}

  // vars
  bool mChanged = false;
  ImVec2 mSize = {0.f,0.f};
  };
//}}}

class cTvUI : public cUI {
public:
  //{{{
    cTvUI(const string& name) : cUI(name) {
    }
  //}}}
  //{{{
  virtual ~cTvUI() {
    }
  //}}}

  //{{{
  void addToDrawList (cApp& app) final {

    if (!mTvLoaded) {
      //{{{  load tv
      const vector <string>& strings = { app.getName() };
      if (!app.getName().empty()) {
        mTvLoaded = true;
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
    //{{{  draw clockButton
    ImGui::SetCursorPos ({ImGui::GetWindowWidth() - 130.f, 0.f});
    clockButton ("clock", app.getPlatform().now(), {110.f,150.f});
    //}}}

    // draw song
    if (isDrawMonoSpaced())
      ImGui::PushFont (app.getMonoFont());

    mDrawTv.draw (isDrawMonoSpaced());

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

  bool mTvLoaded = false;
  cDrawTv mDrawTv;

  static cUI* create (const string& className) { return new cTvUI (className); }
  inline static const bool mRegistered = registerClass ("tv", &create);
  };

// cLayersUI.cpp
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
#include <imgui.h>

#include "cUI.h"
#include "../utils/cLog.h"
#include "../utils/cLoader.h"

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

class cRadioUI : public cUI {
public:
  cRadioUI (const std::string& name) : cUI(name) {}
  virtual ~cRadioUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform) final {
    (void)canvas;
    (void)graphics;
    (void)platform;

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    if (ImGui::Button ("radio1"))
      mLoader.launchLoad (kRadio1);
    if (ImGui::Button ("radio2"))
      mLoader.launchLoad (kRadio2);
    if (ImGui::Button ("radio3"))
      mLoader.launchLoad (kRadio3);
    if (ImGui::Button ("radio4"))
      mLoader.launchLoad (kRadio3);
    if (ImGui::Button ("radio5"))
      mLoader.launchLoad (kRadio5);
    if (ImGui::Button ("radio6"))
      mLoader.launchLoad (kRadio6);
    if (ImGui::Button ("wqxr"))
      mLoader.launchLoad (kWqxr);
    ImGui::End();
    }

private:
  cLoader mLoader;

  //{{{
  static cUI* create (const string& className) {
    return new cRadioUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("radio", &create);
  };

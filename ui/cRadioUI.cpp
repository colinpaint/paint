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

static const vector<string> kRadio4 = {"r4", "a64"};
static const vector<string> kRadio3 = {"r3", "a320"};

class cRadioUI : public cUI {
public:
  cRadioUI (const std::string& name) : cUI(name) {}
  virtual ~cRadioUI() = default;

  void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform) final {
    (void)canvas;
    (void)graphics;
    (void)platform;

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    if (ImGui::Button ("radio3"))
      mLoader.launchLoad (kRadio3);
    if (ImGui::Button ("radio4"))
      mLoader.launchLoad (kRadio4);
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

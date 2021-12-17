// cDemoUI.cpp - will extend to deal with many file types invoking best editUI
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

// app
#include "cDemoApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"

// ui
#include "../ui/cUI.h"

// decoder
#include "../utils/cFileView.h"

// utils
#include "../utils/cLog.h"
#include "../utils/date.h"

using namespace std;
//}}}

class cDemoUI : public cUI {
public:
  //{{{
  cDemoUI (const string& name) : cUI(name) {
    }
  //}}}
  virtual ~cDemoUI() = default;

  void addToDrawList (cApp& app) final {
    bool show_demo_window = true;
    app.getGraphics().drawBackground (cPoint((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

    if (app.getPlatform().hasFullScreen()) {
      ImGui::SameLine();
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      }

     if (app.getPlatform().hasVsync()) {
       // vsync button
       ImGui::SameLine();
       if (toggleButton ("vSync", app.getPlatform().getVsync()))
           app.getPlatform().toggleVsync();

       // fps text
       ImGui::SameLine();
       ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
       }

    ImGui::SameLine();
    ImGui::Text (fmt::format ("{}:{}",
                 ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());

    ImGui::ShowDemoWindow (&show_demo_window);
    }

private:

  //{{{
  static cUI* create (const string& className) {
    return new cDemoUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("demo", &create);
  };

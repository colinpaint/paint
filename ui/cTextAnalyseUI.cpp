// cTextAnalyseUI.cpp
#ifdef _WIN32
//{{{  includes
#include <cstdint>
#include <vector>
#include <string>

// imgui
#include <imgui.h>

// decoder
#include "../decoder/cTextAnalyse.h"

// ui
#include "cUI.h"

// canvas
#include "../canvas/cCanvas.h"

// utils
#include "../utils/cLog.h"

using namespace std;
//}}}

class cTextAnalyseUI : public cUI {
public:
  //{{{
  cTextAnalyseUI (const string& name) : cUI(name) {
    }
  //}}}
  //{{{
  virtual ~cTextAnalyseUI() {
    // close the file mapping object
    delete mTextAnalyse;
    }
  //}}}

  void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform, ImFont* monoFont) final {
    //{{{  unused params
    (void)canvas;
    (void)graphics;
    (void)platform;
    //}}}

    if (!mTextAnalyse)
     mTextAnalyse = new cTextAnalyse ("../brush/cPaintBrush.cpp");

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    ImGui::PushFont(monoFont);

    if (toggleButton ("fold", mShowFolded))
      mShowFolded = !mShowFolded;

    ImGui::SameLine();
    if (toggleButton ("hexDump", mShowHexDump))
      mShowHexDump = !mShowHexDump;

    if (mShowFolded)
      //{{{  analyse
      mTextAnalyse->analyse (
        // callback lambda
        [&](int lineType, const string& info) noexcept {
          (void)lineType;
          ImGui::Text (info.c_str());
          }
        );
      //}}}
    else if (mShowHexDump) {
      //{{{  show hexDump
      ImGui::Indent (10.f);

      mTextAnalyse->resetRead();
      printHex (mTextAnalyse->getReadPtr(), mTextAnalyse->getReadBytesLeft(), 16);

      ImGui::Unindent (10.f);
      }
      //}}}
    else {
      //{{{  show all lines
      mTextAnalyse->resetRead();

      string line;
      while (mTextAnalyse->readLine (line))
        ImGui::Text (line.c_str());
      }
      //}}}

    ImGui::PopFont();
    ImGui::End();
    }

private:
  cTextAnalyse* mTextAnalyse = nullptr;
  ImFont* mMonoFont = nullptr;
  bool mShowFolded = true;
  bool mShowHexDump = false;

  //{{{
  static cUI* create (const string& className) {
    return new cTextAnalyseUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("textAnalyse", &create);
  };
#endif

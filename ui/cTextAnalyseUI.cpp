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

  void addToDrawList (cCanvas* canvas, cGraphics& graphics, cPlatform& platform, ImFont* monoFont) final {
    //{{{  unused params
    (void)canvas;
    (void)graphics;
    (void)platform;
    //}}}

    if (!mTextAnalyse) {
      mTextAnalyse = new cTextAnalyse ("../brush/cPaintBrush.cpp");
      mTextAnalyse->readAndParse();
      }

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    ImGui::PushFont (monoFont);

    if (toggleButton ("fold", mShowFolded))
      mShowFolded = !mShowFolded;
    ImGui::SameLine();
    if (toggleButton ("mixed", mShowMixed))
      mShowMixed = !mShowMixed;
    ImGui::SameLine();
    if (toggleButton ("mixedFull", mShowMixedFull))
      mShowMixedFull = !mShowMixedFull;
    ImGui::SameLine();
    if (toggleButton ("hexDump", mShowHexDump))
      mShowHexDump = !mShowHexDump;

    if (!mShowFolded && !mShowHexDump && !mShowMixed && !mShowMixedFull) {
      //{{{  show unfolded
      string line;
      uint32_t lineNumber;
      uint8_t* ptr;
      uint32_t address;
      uint32_t numBytes;

      mTextAnalyse->resetRead();
      while (mTextAnalyse->readLine (line, lineNumber, ptr, address, numBytes))
        ImGui::Text (fmt::format ("{:3d} {}", lineNumber, line).c_str());
      }
      //}}}
    if (mShowFolded) {
      //{{{  show folded
      mTextAnalyse->resetRead();
      mTextAnalyse->analyse (
        // callback lambda
        [&](const string& info, uint32_t lineNumber) noexcept {
          ImGui::Text (fmt::format ("{:03d} {}", lineNumber, info).c_str());
          }
        );
      }
      //}}}
    if (mShowMixed || mShowMixedFull) {
      //{{{  show mixed
      string line;
      uint32_t lineNumber;
      uint8_t* ptr;
      uint32_t address;
      uint32_t numBytes;

      mTextAnalyse->resetRead();
      while (mTextAnalyse->readLine (line, lineNumber, ptr, address, numBytes)) {
        if (mShowMixedFull) {
          ImGui::Text (fmt::format ("{:4d} {}", lineNumber, line).c_str());
          ImGui::Indent (10.f);
          printHex (ptr, numBytes, 16, address, true);
          ImGui::Unindent (10.f);
          }
        else {
          ImGui::Text (line.c_str());
          printHex (ptr, numBytes, 32, 0, false);
          }
        }
      }
      //}}}
    if (mShowHexDump) {
      //{{{  show hexDump
      mTextAnalyse->resetRead();
      printHex (mTextAnalyse->getReadPtr(), mTextAnalyse->getReadBytesLeft(), 16, 0, true);
      }
      //}}}

    ImGui::PopFont();
    ImGui::End();
    }

private:
  cTextAnalyse* mTextAnalyse = nullptr;
  ImFont* mMonoFont = nullptr;
  bool mShowFolded = true;
  bool mShowMixed = false;
  bool mShowMixedFull = false;
  bool mShowHexDump = false;

  //{{{
  static cUI* create (const string& className) {
    return new cTextAnalyseUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("textAnalyse", &create);
  };
#endif

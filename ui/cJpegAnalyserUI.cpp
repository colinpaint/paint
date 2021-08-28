// cJpegAnalyserUI.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <vector>
#include <string>
#include <chrono>

// imgui
#include <imgui.h>

#include "cUI.h"
#include "../utils/cLog.h"
#include "../decoder/cJpegAnalyser.h"

using namespace std;
using namespace chrono;
//}}}

class cJpegAnalyserUI : public cUI {
public:
  //{{{
  cJpegAnalyserUI (const string& name) : cUI(name) {

    mJpegAnalyser = new cJpegAnalyser ("../piccies/quantel.jpg", 3);
    }
  //}}}
  //{{{
  virtual ~cJpegAnalyserUI() {
    // close the file mapping object
    delete mJpegAnalyser;
    }
  //}}}

  void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform, ImFont* monoFont) final {
    //{{{  unused params
    (void)canvas;
    (void)graphics;
    (void)platform;
    //}}}

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    ImGui::Text (fmt::format ("{} size {}", mJpegAnalyser->getFilename(), mJpegAnalyser->getFileSize()).c_str());

    ImGui::PushFont (monoFont);
    ImGui::Text (mJpegAnalyser->getCreationString().c_str());
    ImGui::Text (mJpegAnalyser->getAccessString().c_str());
    ImGui::Text (mJpegAnalyser->getWriteString().c_str());

    mJpegAnalyser->readHeader (
      [&](uint8_t* ptr, uint32_t offset, uint32_t bytes, const std::string info) noexcept {
        ImGui::Text (fmt::format ("{} {} {}", info, offset, bytes).c_str());

        while (true) {
          string str = fmt::format ("{:04x}: ", offset);
          for (uint32_t curByte = 0; curByte < 16; curByte++) {
            str += fmt::format ("{:02x} ", *ptr++);
            if (--bytes == 0)
              break;
            offset++;
            }
          ImGui::Text (str.c_str());
          if (bytes == 0)
            break;
          }
        }
      );
    ImGui::Text (fmt::format ("{}x{}", mJpegAnalyser->getWidth(), mJpegAnalyser->getHeight()).c_str());

    uint8_t* fileBufferfPtr = mJpegAnalyser->getFilePtr();
    uint32_t offset = 0;
    for (int rows = 0; rows < 64; rows++) {
      string str = fmt::format ("{:04x}: ", offset);
      for (int columns = 0; columns < 16; columns++)
        str += fmt::format ("{:02x} ", *fileBufferfPtr++);
      offset += 64;
      ImGui::Text (str.c_str());
      }

    ImGui::PopFont();
    ImGui::End();
    }

private:
  ImFont* mMonoFont = nullptr;
  cJpegAnalyser* mJpegAnalyser;

  //{{{
  static cUI* create (const string& className) {
    return new cJpegAnalyserUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("jpegAnalyser", &create);
  };

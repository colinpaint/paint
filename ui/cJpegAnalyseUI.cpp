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

// decoder
#include "../decoder/cJpegAnalyse.h"

// ui
#include "cUI.h"

// utils
#include "../utils/cLog.h"

using namespace std;
using namespace chrono;
//}}}
#ifdef _WIN32

class cJpegAnalyseUI : public cUI {
public:
  //{{{
  cJpegAnalyseUI (const string& name) : cUI(name) {

    mJpegAnalyse = new cJpegAnalyse ("../piccies/quantel.jpg", 3);
    }
  //}}}
  //{{{
  virtual ~cJpegAnalyseUI() {
    // close the file mapping object
    delete mJpegAnalyse;
    }
  //}}}

  void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform, ImFont* monoFont) final {
    //{{{  unused params
    (void)canvas;
    (void)graphics;
    (void)platform;
    //}}}

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);

    ImGui::PushFont(monoFont);

    // titles
    ImGui::Text(fmt::format ("{} size {}", mJpegAnalyse->getFilename(), mJpegAnalyse->getFileSize()).c_str());
    ImGui::Text (mJpegAnalyse->getCreationString().c_str());
    ImGui::Text (mJpegAnalyse->getAccessString().c_str());
    ImGui::Text (mJpegAnalyse->getWriteString().c_str());

    // analyse button
    if (toggleButton ("analyse", mAnalyse))
      mAnalyse = !mAnalyse;
    if (mAnalyse) {
      mJpegAnalyse->readHeader (
        [&](const string info, uint8_t* ptr, unsigned offset, unsigned numBytes) noexcept {
          (void)offset;
          ImGui::Text (fmt::format ("{} - {} bytes", info, numBytes).c_str());
          ImGui::Indent (10.f);
          printHex (ptr, numBytes);
          ImGui::Unindent (10.f);
          }
        );

      // body title
      ImGui::Text (fmt::format ("body of imageSize {}x{}",
                                mJpegAnalyse->getWidth(), mJpegAnalyse->getHeight()).c_str());
      }

    // hex dump
    ImGui::Indent (10.f);
    printHex (mJpegAnalyse->getReadPtr(),
              mJpegAnalyse->getReadBytesLeft() < 0x200 ? mJpegAnalyse->getReadBytesLeft() : 0x200);
    ImGui::Unindent (10.f);

    ImGui::PopFont();
    ImGui::End();
    }

private:
  ImFont* mMonoFont = nullptr;
  cJpegAnalyse* mJpegAnalyse;
  bool mAnalyse = false;

  //{{{
  static void printHex (uint8_t* ptr, unsigned numBytes) {

    const unsigned kColumns = 16;

    unsigned offset = 0;
    while (numBytes > 0) {
     string hexString = fmt::format ("{:04x}: ", offset);
     string asciiString;
     for (unsigned curByte = 0; curByte < kColumns; curByte++) {
       if (numBytes > 0) {
         // append byte
         numBytes--;
         uint8_t value = *ptr++;
         hexString += fmt::format ("{:02x} ", value);
         asciiString += (value > 0x20) && (value < 0x80) ? value : 0x2e;
         }
       else // pad row
         hexString += "   ";
       }

     ImGui::Text ((hexString + " " + asciiString).c_str());
     offset += kColumns;
     }
   }
  //}}}

  //{{{
  static cUI* create (const string& className) {
    return new cJpegAnalyseUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("jpegAnalyse", &create);
  };
#endif

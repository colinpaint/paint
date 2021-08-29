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

    ImGui::PushFont(monoFont);

    // titles
    ImGui::Text(fmt::format ("{} size {}", mJpegAnalyser->getFilename(), mJpegAnalyser->getFileSize()).c_str());
    ImGui::Text (mJpegAnalyser->getCreationString().c_str());
    ImGui::Text (mJpegAnalyser->getAccessString().c_str());
    ImGui::Text (mJpegAnalyser->getWriteString().c_str());

    // analyse button
    if (toggleButton ("analyse", mAnalyse))
      mAnalyse = !mAnalyse;
    if (mAnalyse) {
      mJpegAnalyser->readHeader (
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
                                mJpegAnalyser->getWidth(), mJpegAnalyser->getHeight()).c_str());
      }

    // hex dump
    ImGui::Indent (10.f);
    printHex (mJpegAnalyser->getReadPtr(),
              mJpegAnalyser->getReadBytesLeft() < 0x200 ? mJpegAnalyser->getReadBytesLeft() : 0x200);
    ImGui::Unindent (10.f);

    ImGui::PopFont();
    ImGui::End();
    }

private:
  ImFont* mMonoFont = nullptr;
  cJpegAnalyser* mJpegAnalyser;
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
    return new cJpegAnalyserUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("jpegAnalyser", &create);
  };

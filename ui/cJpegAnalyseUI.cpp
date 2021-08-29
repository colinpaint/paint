// cJpegAnalyserUI.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#include <windows.h>

#include <cstdint>
#include <vector>
#include <string>

// imgui
#include <imgui.h>

// decoder
#include "../decoder/cJpegAnalyse.h"

// ui
#include "cUI.h"

// canvas
#include "../canvas/cCanvas.h"

// utils
#include "../utils/cLog.h"

using namespace std;
//}}}
#ifdef _WIN32

class cJpegAnalyseUI : public cUI {
public:
  //{{{
  cJpegAnalyseUI (const string& name) : cUI(name) {
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
    (void)graphics;
    (void)platform;
    //}}}

    if (!mJpegAnalyse)
      mJpegAnalyse = new cJpegAnalyse (canvas.getName(), 3);

    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    ImGui::PushFont(monoFont);

    // buttons
    ImGui::Text(fmt::format ("{} size {}", mJpegAnalyse->getFilename(), mJpegAnalyse->getFileSize()).c_str());
    if (toggleButton ("fileTimes", mShowFileTimes))
      mShowFileTimes = !mShowFileTimes;
    ImGui::SameLine();
    if (toggleButton ("analyse", mShowAnalyse))
      mShowAnalyse = !mShowAnalyse;
    ImGui::SameLine();
    if (toggleButton ("hexDump", mShowHexDump))
      mShowHexDump = !mShowHexDump;

    if (mShowFileTimes) {
      //{{{  show fileTimes
      ImGui::Indent (10.f);
      ImGui::Text (mJpegAnalyse->getCreationString().c_str());
      ImGui::Text (mJpegAnalyse->getAccessString().c_str());
      ImGui::Text (mJpegAnalyse->getWriteString().c_str());
      ImGui::Unindent (10.f);
      }
      //}}}

    mJpegAnalyse->resetReadBytes();
    if (mShowAnalyse) {
      mJpegAnalyse->readHeader (
        //{{{  jpegTag lambda
        [&](const string info, uint8_t* ptr, unsigned offset, unsigned numBytes) noexcept {
          //{{{  unused params
          (void)offset;
          //}}}

          ImGui::Text (fmt::format ("{} - {} bytes", info, numBytes).c_str());

          if (numBytes && mShowHexDump) {
            ImGui::Indent (10.f);
            printHex (ptr, numBytes);
            ImGui::Unindent (10.f);
            }
          },
        //}}}
        //{{{  exifTag lambda
        [&](const string info, uint8_t* ptr, unsigned offset, unsigned numBytes) noexcept {
          //{{{  unused params
          (void)offset;
          //}}}
          ImGui::Indent (10.f);
          ImGui::Text (fmt::format ("exif {}", info, numBytes).c_str());

          if (numBytes && mShowHexDump) {
            ImGui::Indent (10.f);
            printHex (ptr, numBytes);
            ImGui::Unindent (10.f);
            }

          ImGui::Unindent (10.f);
          }
        //}}}
        );

      ImGui::Text (fmt::format ("image size {}x{} - body {} bytes",
                   mJpegAnalyse->getWidth(), mJpegAnalyse->getHeight(),
                   mJpegAnalyse->getReadBytesLeft()).c_str());
      }

    if (mShowHexDump) {
      //{{{  show hexDump
      ImGui::Indent (10.f);
      printHex (mJpegAnalyse->getReadPtr(),
                mJpegAnalyse->getReadBytesLeft() < 0x200 ? mJpegAnalyse->getReadBytesLeft() : 0x200);
      ImGui::Unindent (10.f);
      }
      //}}}

    ImGui::PopFont();
    ImGui::End();
    }

private:
  cJpegAnalyse* mJpegAnalyse;
  ImFont* mMonoFont = nullptr;

  bool mShowFileTimes = false;
  bool mShowAnalyse = false;
  bool mShowHexDump = false;

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
         asciiString += (value > 0x20) && (value < 0x80) ? value : '.';
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

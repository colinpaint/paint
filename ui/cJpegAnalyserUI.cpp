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

class cJpegAnalyserUI : public cUI {
public:
  //{{{
  cJpegAnalyserUI (const std::string& name) : cUI(name) {
    fileHandle = CreateFile (mFilename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    mapHandle = CreateFileMapping (fileHandle, NULL, PAGE_READONLY, 0, 0, NULL);
    fileBuf = (uint8_t*)MapViewOfFile (mapHandle, FILE_MAP_READ, 0, 0, 0);
    fileBufLen = GetFileSize (fileHandle, NULL);

    FILETIME creationTime;
    FILETIME lastAccessTime;
    FILETIME lastWriteTime;
    GetFileTime (fileHandle, &creationTime, &lastAccessTime, &lastWriteTime);

    mCreationTimePoint = getFileTimePoint (creationTime);
    mLastAccessTimePoint = getFileTimePoint (lastAccessTime);
    mLastWriteTimePoint = getFileTimePoint (lastWriteTime);
    }
  //}}}
  //{{{
  virtual ~cJpegAnalyserUI() {
    // close the file mapping object
    UnmapViewOfFile (fileBuf);
    CloseHandle (mapHandle);
    CloseHandle (fileHandle);
    }
  //}}}

  //{{{
  void addToDrawList (cCanvas& canvas, cGraphics& graphics, cPlatform& platform, ImFont* monoFont) final {

    //{{{  unused params
    (void)canvas;
    (void)graphics;
    (void)platform;
    //}}}
    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);

    cJpegAnalyser jpegAnalyser (3, fileBuf);
    jpegAnalyser.readHeader();

    ImGui::Text (fmt::format ("{} {}bytes {}x{}",
                              mFilename.c_str(), fileBufLen,
                              jpegAnalyser.getWidth(), jpegAnalyser.getHeight()).c_str());

    ImGui::Text (date::format ("create - %H:%M:%S %a %d %b %y", chrono::floor<chrono::seconds>(mCreationTimePoint)).c_str());
    ImGui::Text (date::format ("access - %H:%M:%S %a %d %b %y", chrono::floor<chrono::seconds>(mLastAccessTimePoint)).c_str());
    ImGui::Text (date::format ("write  - %H:%M:%S %a %d %b %y", chrono::floor<chrono::seconds>(mLastWriteTimePoint)).c_str());

    ImGui::PushFont (monoFont);
    int address = 0;
    for (int j = 0; j < 16; j++) {
      string str = fmt::format ("{:04x}: ", address);
      for (int i = 0; i < 16; i++)
        str += fmt::format ("{:02x} ", fileBuf[address++]);
      ImGui::Text (str.c_str());
      }
    ImGui::PopFont();

    ImGui::End();
    }
  //}}}

private:
  //{{{
  static chrono::system_clock::time_point getFileTimePoint (FILETIME fileTime) {

    // filetime_duration has the same layout as FILETIME; 100ns intervals
    using filetime_duration = chrono::duration<int64_t, ratio<1, 10'000'000>>;

    // January 1, 1601 (NT epoch) - January 1, 1970 (Unix epoch):
    constexpr chrono::duration<int64_t> nt_to_unix_epoch { INT64_C(-11644473600) };

    const filetime_duration asDuration{static_cast<int64_t> (
        (static_cast<uint64_t>((fileTime).dwHighDateTime) << 32) | (fileTime).dwLowDateTime)};

    const auto withUnixEpoch = asDuration + nt_to_unix_epoch;

    return chrono::system_clock::time_point { duration_cast<chrono::system_clock::duration>(withUnixEpoch) };
    }
  //}}}

  ImFont* mMonoFont = nullptr;

  string mFilename = "../piccies/tv.jpg";
  HANDLE fileHandle;
  HANDLE mapHandle;
  uint8_t* fileBuf;
  DWORD fileBufLen;

  std::string mExifTimeString;
  std::chrono::time_point<std::chrono::system_clock> mExifTimePoint;
  std::chrono::time_point<std::chrono::system_clock> mCreationTimePoint;
  std::chrono::time_point<std::chrono::system_clock> mLastAccessTimePoint;
  std::chrono::time_point<std::chrono::system_clock> mLastWriteTimePoint;

  //{{{
  static cUI* create (const string& className) {
    return new cJpegAnalyserUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("jpegAnalyser", &create);
  };

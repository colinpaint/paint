// test.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <mutex>
#include <shared_mutex>

#include <sys/stat.h>

// utils
#include "../date/include/date/date.h"
#include "../common/basicTypes.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

// dvb
#include "../dvb/cTransportStream.h"

// decoders
//{{{  include libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  #include "libavutil/motion_vector.h"
  #include "libavutil/frame.h"
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}
#include "../decoders/cVideoFrame.h"
#include "../decoders/cFFmpegVideoDecoder.h"
#include "../decoders/cVideoRender.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/myImgui.h"

using namespace std;
using namespace utils;
//}}}

//{{{
class cFilePlayer {
public:
  cFilePlayer (string fileName) : mFileName(cFileUtils::resolve (fileName)) {}
  //{{{
  virtual ~cFilePlayer() {
    mAudioPesMap.clear();
    mGopMap.clear();
    }
  //}}}

  string getFileName() const { return mFileName; }
  cTransportStream::cService* getService() { return mService; }
  cVideoRender* getVideoRender() { return mVideoRender; }
  int64_t getPlayPts() const { return mPlayPts; }

  void togglePlay() { mPlaying = !mPlaying; }
  void skipPlay (int64_t skipPts) { mPlayPts += skipPts; }
  //{{{
  void skipIframe (int64_t inc) {

    auto it = mGopMap.upper_bound (mPlayPts);
    int64_t upperPts = it->first;

    if (inc <= 0)
      --it;
    if (inc < 0)
      --it;
    mPlayPts = it->first;

    cLog::log (LOGINFO, fmt::format ("skipI {} {} {}",
                                     inc, getPtsString (upperPts), getPtsString (it->first)));
    }
  //}}}

  //{{{
  void read() {

    FILE* file = fopen (mFileName.c_str(), "rb");
    if (!file) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("cFilePlayer::analyse to open {}", mFileName));
      return;
      }
      //}}}
    //{{{  get fileSize
    #ifdef _WIN32
      struct _stati64 st;
      if (_stat64 (mFileName.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #else
      struct stat st;
      if (stat (mFileName.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #endif
    //}}}

    thread ([=]() {
      cLog::setThreadName ("anal");

      mTransportStream = new cTransportStream (
        {"anal", 0, {}, {}}, nullptr,
        // newService lambda, !!! hardly used !!!
        [&](cTransportStream::cService& service) noexcept {
          mService = &service;
          mVideoRender = new cVideoRender (false, 1000, mService->getVideoStreamTypeId(), mService->getVideoPid());
          },

         // pes lambda
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo) noexcept {
          if (pidInfo.getPid() == service.getVideoPid()) {
            uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
            memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());

            char frameType = cDvbUtils::getFrameType (buffer, pidInfo.getBufSize(), true);
            sPes pes (buffer, pidInfo.getBufSize(), pidInfo.getPts(), frameType);
            if (frameType == 'I') // add gop
              mGopMap.emplace (pidInfo.getPts(), cGop(pes));
            else if (!mGopMap.empty()) // add pes to last gop
              mGopMap.rbegin()->second.addPes (pes);
            //{{{  debug
            cLog::log (LOGINFO, fmt::format ("{}:{:2d} {:6d} pts:{}",
                                             frameType,
                                             mGopMap.empty() ? 0 : mGopMap.rbegin()->second.getSize(),
                                             pidInfo.getBufSize(), getFullPtsString (pidInfo.getPts()) ));
            //}}}
            //if (frameType == 'I')
              if (!mGopMap.empty() && mVideoRender) {
                //{{{  decode
                mVideoRender->decodePes (buffer, pidInfo.getBufSize(), pidInfo.getPts(), frameType);
                mPlayPts = pidInfo.getPts();
                //this_thread::sleep_for (40ms);
                }
                //}}}

            while (!mPlaying)
              this_thread::sleep_for (40ms);
            }
          //{{{  pes unused
          //else if (pidInfo.getPid() == service.getAudioPid())
          //  cLog::log (LOGINFO, fmt::format ("- A pts:{} size:{}", getPtsString (pidInfo.getPts()), pidInfo.getBufSize()));
          //else if (pidInfo.getPid() == service.getSubtitlePid())
          //  cLog::log (LOGINFO, fmt::format ("  - S pts:{} size:{}", getPtsString (pidInfo.getPts()), pidInfo.getBufSize()));
          //else
          // cLog::log (LOGERROR, fmt::format ("cFilePlayer analyse addPes - unknown pid:{}", pidInfo.getPid()));
          //}}}
          }
        );

      // file read
      size_t chunkSize = 188 * 256;
      uint8_t* chunk = new uint8_t[chunkSize];
      auto now = chrono::system_clock::now();
      while (true) {
        size_t bytesRead = fread (chunk, 1, chunkSize, file);
        if (bytesRead > 0)
          mTransportStream->demux (chunk, bytesRead);
        else
          break;
        }
      //{{{  report totals
      cLog::log (LOGINFO, fmt::format ("{}m took {}ms",
        mFileSize/1000000,
        chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count()));

      size_t numVidPes = 0;
      size_t longestGop = 0;
      for (auto& gop : mGopMap) {
        longestGop = max (longestGop, gop.second.getSize());
        numVidPes += gop.second.getSize();
        }

      cLog::log (LOGINFO, fmt::format ("- vid:{:6d} {} to {} gop:{} longest:{}",
                                       numVidPes,
                                       getFullPtsString (mGopMap.begin()->second.getFirstPts()),
                                       getFullPtsString (mGopMap.rbegin()->second.getLastPts()),
                                       mGopMap.size(), longestGop));

      cLog::log (LOGINFO, fmt::format ("- aud:{:6d} {} to {}",
                                       mAudioPesMap.size(),
                                       getFullPtsString (mAudioPesMap.begin()->first),
                                       getFullPtsString (mAudioPesMap.rbegin()->first)));
      //}}}

      delete[] chunk;
      fclose (file);

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
  //}}}

private:
  //{{{
  struct sPes {
  public:
    sPes (uint8_t* data, uint32_t size, int64_t pts, char frameType) :
      mData(data), mSize(size), mPts(pts), mFrameType(frameType) {}

    uint8_t* mData;
    uint32_t mSize;
    int64_t mPts;
    char mFrameType;
    };
  //}}}
  //{{{
  class cGop {
  public:
    cGop (sPes pes) {
      mPesVector.push_back (pes);
      }
    virtual ~cGop() {
      mPesVector.clear();
      }

    size_t getSize() const { return mPesVector.size(); }
    int64_t getFirstPts() const { return mPesVector.front().mPts; }
    int64_t getLastPts() const { return mPesVector.back().mPts; }

    void addPes (sPes pes) {
      mPesVector.push_back (pes);
      }

    void load (cVideoRender* videoRender, int64_t loadPts, int64_t pts, const string& title) {
      cLog::log (LOGINFO, fmt::format ("{} {} {} gopFirst:{}",
                                       title, mPesVector.size(), getPtsString (loadPts), getPtsString (pts) ));

      for (auto it = mPesVector.begin(); it != mPesVector.end(); ++it)
        videoRender->decodePes (it->mData, it->mSize, it->mPts, it->mFrameType);
      }

  private:
    vector <sPes> mPesVector;
    };
  //}}}

  string mFileName;
  size_t mFileSize = 0;

  cTransportStream* mTransportStream = nullptr;
  cTransportStream::cService* mService = nullptr;

  // pes
  shared_mutex mAudioMutex;
  map <int64_t,sPes> mAudioPesMap;

  int64_t mNumVideoPes = 0;
  map <int64_t,cGop> mGopMap;

  // render
  cVideoRender* mVideoRender = nullptr;
  int64_t mPlayPts = -1;
  bool mPlaying = true;
  };
//}}}
//{{{
class cTestApp : public cApp {
public:
  cTestApp (cApp::cOptions* options, iUI* ui) : cApp ("Test", options, ui), mOptions(options) {}
  virtual ~cTestApp() {}

  void addFile (const string& fileName, cApp::cOptions* options) {
    mFilePlayer = new cFilePlayer (fileName);
    mFilePlayer->read();
    }

  cApp::cOptions* getOptions() { return mOptions; }
  cFilePlayer* getFilePlayer() { return mFilePlayer; }

  void drop (const vector<string>& dropItems) {
    for (auto& item : dropItems) {
      cLog::log (LOGINFO, fmt::format ("cPlayerApp::drop {}", item));
      addFile (item, mOptions);
      }
    }

private:
  cApp::cOptions* mOptions;
  cFilePlayer* mFilePlayer = nullptr;
  };
//}}}
//{{{
class cView {
public:
  cView (cTransportStream::cService* service) : mService(service) {}
  ~cView() = default;

  void draw (cTestApp& testApp, cTextureShader* videoShader) {
    float layoutScale;
    cVec2 layoutPos = getLayout (0, 1, layoutScale);
    float viewportWidth = ImGui::GetWindowWidth();
    float viewportHeight = ImGui::GetWindowHeight();
    mSize = {layoutScale * viewportWidth, layoutScale * viewportHeight};
    mTL = {(layoutPos.x * viewportWidth) - mSize.x*0.5f, (layoutPos.y * viewportHeight) - mSize.y*0.5f};
    mBR = {(layoutPos.x * viewportWidth) + mSize.x*0.5f, (layoutPos.y * viewportHeight) + mSize.y*0.5f};

    ImGui::SetCursorPos (mTL);
    ImGui::BeginChild (fmt::format ("view##{}", mService->getSid()).c_str(), mSize,
                       ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                       ImGuiWindowFlags_NoBackground);

    if (testApp.getFilePlayer()) {
      cVideoRender* videoRender = testApp.getFilePlayer()->getVideoRender();
      if (videoRender) {
        cVideoFrame* videoFrame = videoRender->getVideoFrameAtOrAfterPts (testApp.getFilePlayer()->getPlayPts());
        if (videoFrame) {
          //{{{  draw video
          cMat4x4 model = cMat4x4();
          model.setTranslate ({(layoutPos.x - (0.5f * layoutScale)) * viewportWidth,
                               ((1.f-layoutPos.y) - (0.5f * layoutScale)) * viewportHeight});
          model.size ({layoutScale * viewportWidth / videoFrame->getWidth(),
                       layoutScale * viewportHeight / videoFrame->getHeight()});
          cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);
          videoShader->use();
          videoShader->setModelProjection (model, projection);

          // texture
          cTexture& texture = videoFrame->getTexture (testApp.getGraphics());
          texture.setSource();

          // ensure quad is created
          if (!mVideoQuad)
            mVideoQuad = testApp.getGraphics().createQuad (videoFrame->getSize());

          // draw quad
          mVideoQuad->draw();
          }
          //}}}
        }
      //{{{  title, playPts
      ImGui::PushFont (testApp.getLargeFont());

      string title = testApp.getFilePlayer()->getFileName();
      ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
      ImGui::SetCursorPos (pos);
      ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
      ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
      ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());

      string ptsString = getPtsString (testApp.getFilePlayer()->getPlayPts());
      pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
      ImGui::SetCursorPos (pos);
      ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsString.c_str());
      ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
      ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsString.c_str());

      ImGui::PopFont();
      //}}}
      }
    ImGui::EndChild();
    }

private:
  //{{{
  cVec2 getLayout (size_t index, size_t numViews, float& scale) {
  // return layout scale, and position as fraction of viewPort

    scale = (numViews <= 1) ? 1.f :
              ((numViews <= 4) ? 1.f/2.f :
                ((numViews <= 9) ? 1.f/3.f :
                  ((numViews <= 16) ? 1.f/4.f : 1.f/5.f)));

    switch (numViews) {
      //{{{
      case 2: // 2x1
        switch (index) {
          case 0: return { 1.f / 4.f, 0.5f };
          case 1: return { 3.f / 4.f, 0.5f };
          }
        return { 0.5f, 0.5f };
      //}}}

      case 3:
      //{{{
      case 4: // 2x2
        switch (index) {
          case 0: return { 1.f / 4.f, 1.f / 4.f };
          case 1: return { 3.f / 4.f, 1.f / 4.f };

          case 2: return { 1.f / 4.f, 3.f / 4.f };
          case 3: return { 3.f / 4.f, 3.f / 4.f };
          }
        return { 0.5f, 0.5f };
      //}}}

      case 5:
      //{{{
      case 6: // 3x2
        switch (index) {
          case 0: return { 1.f / 6.f, 2.f / 6.f };
          case 1: return { 3.f / 6.f, 2.f / 6.f };
          case 2: return { 5.f / 6.f, 2.f / 6.f };

          case 3: return { 1.f / 6.f, 4.f / 6.f };
          case 4: return { 3.f / 6.f, 4.f / 6.f };
          case 5: return { 5.f / 6.f, 4.f / 6.f };
          }
        return { 0.5f, 0.5f };
      //}}}

      case 7:
      case 8:
      //{{{
      case 9: // 3x3
        switch (index) {
          case 0: return { 1.f / 6.f, 1.f / 6.f };
          case 1: return { 3.f / 6.f, 1.f / 6.f };
          case 2: return { 5.f / 6.f, 1.f / 6.f };

          case 3: return { 1.f / 6.f, 0.5f };
          case 4: return { 3.f / 6.f, 0.5f };
          case 5: return { 5.f / 6.f, 0.5f };

          case 6: return { 1.f / 6.f, 5.f / 6.f };
          case 7: return { 3.f / 6.f, 5.f / 6.f };
          case 8: return { 5.f / 6.f, 5.f / 6.f };
          }
        return { 0.5f, 0.5f };
      //}}}

      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
      //{{{
      case 16: // 4x4
        switch (index) {
          case  0: return { 1.f / 8.f, 1.f / 8.f };
          case  1: return { 3.f / 8.f, 1.f / 8.f };
          case  2: return { 5.f / 8.f, 1.f / 8.f };
          case  3: return { 7.f / 8.f, 1.f / 8.f };

          case  4: return { 1.f / 8.f, 3.f / 8.f };
          case  5: return { 3.f / 8.f, 3.f / 8.f };
          case  6: return { 5.f / 8.f, 3.f / 8.f };
          case  7: return { 7.f / 8.f, 3.f / 8.f };

          case  8: return { 1.f / 8.f, 5.f / 8.f };
          case  9: return { 3.f / 8.f, 5.f / 8.f };
          case 10: return { 5.f / 8.f, 5.f / 8.f };
          case 11: return { 7.f / 8.f, 5.f / 8.f };

          case 12: return { 1.f / 8.f, 7.f / 8.f };
          case 13: return { 3.f / 8.f, 7.f / 8.f };
          case 14: return { 5.f / 8.f, 7.f / 8.f };
          case 15: return { 7.f / 8.f, 7.f / 8.f };
          }
        return { 0.5f, 0.5f };
      //}}}

      default: // 1x1
        return { 0.5f, 0.5f };
      }
    }
  //}}}

  // vars
  cTransportStream::cService* mService;

  // video
  ImVec2 mSize;
  ImVec2 mTL;
  ImVec2 mBR;
  cQuad* mVideoQuad = nullptr;
  };
//}}}
//{{{
class cTestUI : public cApp::iUI {
public:
  virtual ~cTestUI() = default;

  void draw (cApp& app) {
    ImGui::SetKeyboardFocusHere();
    app.getGraphics().clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                              (int32_t)ImGui::GetIO().DisplaySize.y});

    // draw UI
    ImGui::SetNextWindowPos ({0.f,0.f});
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    ImGui::Begin ("test", nullptr, ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoBackground);

    cTestApp& testApp = (cTestApp&)app;
    if (!mVideoShader)
      mVideoShader = testApp.getGraphics().createTextureShader (cTexture::eYuv420);

    cFilePlayer* filePlayer = testApp.getFilePlayer();
    if (filePlayer && filePlayer->getService()) {
      if (!mView)
        mView = new cView (filePlayer->getService());
      mView->draw (testApp, mVideoShader);

      // draw menu
      ImGui::SetCursorPos ({0.f, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeight() * 1.5f});
      ImGui::BeginChild ("menu", {0.f, ImGui::GetTextLineHeight() * 1.5f},
                         ImGuiChildFlags_None,
                         ImGuiWindowFlags_NoBackground);

      ImGui::SetCursorPos ({0.f,0.f});
      //{{{  draw fullScreen button
      ImGui::SameLine();
      if (toggleButton ("full", testApp.getPlatform().getFullScreen()))
        testApp.getPlatform().toggleFullScreen();
      //}}}
      //{{{  draw frameRate info
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      //}}}
      ImGui::EndChild();
      }
    ImGui::End();
    keyboard (testApp);
    }

private:
  //{{{
  void keyboard (cTestApp& testApp) {

    //{{{
    struct sActionKey {
      bool mAlt;
      bool mControl;
      bool mShift;
      ImGuiKey mGuiKey;
      function <void()> mActionFunc;
      };
    //}}}
    const vector<sActionKey> kActionKeys = {
    //  alt    control shift  ImGuiKey             function
      { false, false,  false, ImGuiKey_Space,      [this,&testApp]{ testApp.getFilePlayer()->togglePlay(); }},

      { false, false,  false, ImGuiKey_LeftArrow,  [this,&testApp]{ testApp.getFilePlayer()->skipPlay (-90000/25); }},
      { false, false,  false, ImGuiKey_RightArrow, [this,&testApp]{ testApp.getFilePlayer()->skipPlay (90000/25); }},
      { false, true,   false, ImGuiKey_LeftArrow,  [this,&testApp]{ testApp.getFilePlayer()->skipPlay (-90000); }},
      { false, true,   false, ImGuiKey_RightArrow, [this,&testApp]{ testApp.getFilePlayer()->skipPlay (90000); }},

      { false, false,  false, ImGuiKey_UpArrow,    [this,&testApp]{ testApp.getFilePlayer()->skipIframe (-1); }},
      { false, false,  false, ImGuiKey_DownArrow,  [this,&testApp]{ testApp.getFilePlayer()->skipIframe (1); }},
    };

    ImGui::GetIO().WantTextInput = true;
    ImGui::GetIO().WantCaptureKeyboard = true;

    // action keys
    bool altKey = ImGui::GetIO().KeyAlt;
    bool controlKey = ImGui::GetIO().KeyCtrl;
    bool shiftKey = ImGui::GetIO().KeyShift;
    for (auto& actionKey : kActionKeys)
      if (ImGui::IsKeyPressed (actionKey.mGuiKey) &&
          (actionKey.mAlt == altKey) &&
          (actionKey.mControl == controlKey) &&
          (actionKey.mShift == shiftKey)) {
        actionKey.mActionFunc();
        break;
        }

    ImGui::GetIO().InputQueueCharacters.resize (0);
    }
  //}}}

  // vars
  cView* mView;
  cTextureShader* mVideoShader = nullptr;
  };
//}}}

// main
int main (int numArgs, char* args[]) {

  // options
  string fileName;
  cApp::cOptions* options = new cApp::cOptions();
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) // found cApp option
      ;
    else
      fileName = param;
    }

  // log
  cLog::init (options->mLogLevel);

  // launch
  cTestApp testApp (options, new cTestUI());
  testApp.addFile (fileName, options);
  testApp.mainUILoop();

  return EXIT_SUCCESS;
  }

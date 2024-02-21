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
#include "../decoders/cVideoRender.h"
#include "../decoders/cFFmpegVideoDecoder.h"

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

  //{{{
  void start() {
    analyseThread();
    }
  //}}}

private:
  //{{{
  struct sPes {
  public:
    sPes (uint8_t* data, uint32_t size, int64_t pts, int64_t dts, char type = '?') :
      mData(data), mSize(size), mPts(pts), mDts(dts), mType(type) {}

    uint8_t* mData;
    uint32_t mSize;
    int64_t mPts;
    int64_t mDts;
    char mType;
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

      for (auto& it = mPesVector.begin(); it != mPesVector.end(); ++it)
        videoRender->decodePes (it->mData, it->mSize, it->mPts, it->mDts);
      }

  private:
    vector <sPes> mPesVector;
    };
  //}}}

  //{{{
  bool analyseThread() {
  // launch analyser thread

    FILE* file = fopen (mFileName.c_str(), "rb");
    if (!file) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("cFilePlayer::analyse to open {}", mFileName));
      return false;
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
        [&](cTransportStream::cService& service) noexcept { mService = &service; },
        //{{{  pes lambda
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo) noexcept {
          if (pidInfo.getPid() == service.getVideoPid()) {
            // add video pes, take copy of pes
            uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
            memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());

            char frameType = cDvbUtils::getFrameType (buffer, pidInfo.getBufSize(), true);
            if ((frameType == 'I') || mGopMap.empty())
              mGopMap.emplace (pidInfo.getPts(), cGop(sPes(buffer, pidInfo.getBufSize(),
                                                           pidInfo.getPts(), pidInfo.getDts(), frameType)));
            else
              mGopMap.rbegin()->second.addPes (sPes(buffer, pidInfo.getBufSize(),
                                                    pidInfo.getPts(), pidInfo.getDts(), frameType));
            cLog::log (LOGINFO, fmt::format ("V {}:{:2d} dts:{} pts:{} size:{}",
                                             frameType,
                                             mGopMap.empty() ? 0 : mGopMap.rbegin()->second.getSize(),
                                             getFullPtsString (pidInfo.getDts()),
                                             getFullPtsString (pidInfo.getPts()),
                                             pidInfo.getBufSize()
                                             ));

            }

          else if (pidInfo.getPid() == service.getAudioPid()) {
            // add audio pes, take copy of pes
            uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
            memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());
            cLog::log (LOGINFO, fmt::format ("A pts:{} size:{}",
                                             getFullPtsString (pidInfo.getPts()), pidInfo.getBufSize()));

            unique_lock<shared_mutex> lock (mAudioMutex);
            mAudioPesMap.emplace (pidInfo.getPts(),
                                  sPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
            }
          else if (pidInfo.getPid() == service.getSubtitlePid()) {
            }
          else
            cLog::log (LOGERROR, fmt::format ("cFilePlayer analyse addPes - unknown pid:{}", pidInfo.getPid()));
          }
        //}}}
        );

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

      while (true)
        this_thread::sleep_for (1s);

      delete[] chunk;
      fclose (file);

      cLog::log (LOGERROR, "exit");
      }).detach();

    return true;
    }
  //}}}
  //{{{
  void videoLoaderThread() {
  // launch videoLoader thread

    thread ([=]() {
      cLog::setThreadName ("vidL");

      //{{{  wait for first video pes
      while (mGopMap.begin() == mGopMap.end()) {
        cLog::log (LOGINFO, fmt::format ("videoLoader start wait"));
        this_thread::sleep_for (100ms);
        }
      //}}}
      mVideoRender = new cVideoRender (false, 100, 0,
                                       mService->getVideoStreamTypeId(), mService->getVideoPid());
      int64_t playPts = 0;
      mGopMap.begin()->second.load (mVideoRender, playPts, mGopMap.begin()->first, "first");

      while (true) {
        int64_t loadPts = playPts;
        auto nextIt = mGopMap.upper_bound (loadPts);
        auto it = nextIt;
        if (it != mGopMap.begin())
          --it;

        if (!mVideoRender->isFrameAtPts (loadPts)) {
          it->second.load (mVideoRender, loadPts, it->first, "load");
          continue;
          }

        // ensure next loaded
        if (nextIt != mGopMap.end()) {
          if (!mVideoRender->isFrameAtPts (nextIt->first)) {
            nextIt->second.load (mVideoRender, loadPts, nextIt->first, "next");
            continue;
            }
          }

        // ensure prev loaded
        //if (--it != mGopMap.begin()) {
        //  if (!mVideoRender->isFrameAtPts (it->first)) {
        //    it->second.load (mVideoRender, loadPts, it->first, "prev");
        //    continue;
        //    }
        //  }

        // wait for change
        this_thread::sleep_for (1ms);
        }

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
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
  };
//}}}
//{{{
class cTestApp : public cApp {
public:
  cTestApp (cApp::cOptions* options, iUI* ui) : cApp ("Test", options, ui), mOptions(options) {}
  virtual ~cTestApp() {}

  void addFile (const string& fileName, cApp::cOptions* options) {
    mFilePlayer = new cFilePlayer (fileName);
    mFilePlayer->start();
    }

  cApp::cOptions* getOptions() { return mOptions; }
  cFilePlayer* getFilePlayer() { return mFilePlayer; }

  void drop (const vector<string>& dropItems) {}

private:
  cApp::cOptions* mOptions;
  cFilePlayer* mFilePlayer = nullptr;
  };
//}}}
//{{{
class cView {
public:
  cView (cTransportStream::cService* service) : mService(service) {}
  //{{{
  ~cView() {
    delete mVideoQuad;

    }
  //}}}

  //{{{
  void draw (cTestApp& testApp, bool selectFull, size_t viewIndex, size_t numViews,
             cTextureShader* videoShader, cTextureShader* subtitleShader) {

    float layoutScale;
    cVec2 layoutPos = getLayout (viewIndex, numViews, layoutScale);
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

    // get videoFrame
    //cVideoFrame* videoFrame = videoRender->getVideoFrameAtOrAfterPts (playPts);
    //if (videoFrame) {
      //{{{  draw video
      //cMat4x4 model = cMat4x4();
      //model.setTranslate ({(layoutPos.x - (0.5f * layoutScale)) * viewportWidth,
                           //((1.f-layoutPos.y) - (0.5f * layoutScale)) * viewportHeight});
      //model.size ({layoutScale * viewportWidth / videoFrame->getWidth(),
                   //layoutScale * viewportHeight / videoFrame->getHeight()});
      //cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);
      //videoShader->use();
      //videoShader->setModelProjection (model, projection);

      //// texture
      //cTexture& texture = videoFrame->getTexture (testApp.getGraphics());
      //texture.setSource();

      //// ensure quad is created
      //if (!mVideoQuad)
        //mVideoQuad = testApp.getGraphics().createQuad (videoFrame->getSize());

      //// draw quad
      //mVideoQuad->draw();

      //}}}

    ImGui::PushFont (testApp.getLargeFont());
    string title = testApp.getFilePlayer()->getFileName();
    ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
    ImGui::SetCursorPos (pos);
    ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
    ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
    ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
    ImGui::PopFont();

    ImGui::EndChild();
    }
  //}}}

private:
  enum eSelect { eUnselected, eSelected, eSelectedFull };
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
  eSelect mSelect = eUnselected;

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

  //{{{
  void draw (cApp& app) {

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
      mView->draw (testApp, true, 0, true, mVideoShader, nullptr);

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
      if (testApp.getPlatform().hasVsync()) {
        //{{{  draw vsync button
        ImGui::SameLine();
        if (toggleButton ("vsync", testApp.getPlatform().getVsync()))
          testApp.getPlatform().toggleVsync();
        }
        //}}}
      //{{{  draw frameRate info
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      //}}}

      ImGui::EndChild();
      }

    ImGui::End();
    }
  //}}}

private:

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

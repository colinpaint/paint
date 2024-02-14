// player.cpp
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

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/myImgui.h"

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
#include "../decoders/cAudioFrame.h"
#include "../decoders/cVideoFrame.h"
#include "../decoders/cFFmpegVideoDecoder.h"
#include "../decoders/cSubtitleFrame.h"
#include "../decoders/cSubtitleImage.h"

// dvb
#include "../dvb/cTransportStream.h"
#include "../dvb/cRender.h"
#include "../dvb/cVideoRender.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cPlayer.h"

using namespace std;
//}}}

//{{{
class cPlayerOptions : public cApp::cOptions,
                       public cTransportStream::cOptions,
                       public cRender::cOptions,
                       public cAudioRender::cOptions,
                       public cVideoRender::cOptions,
                       public cFFmpegVideoDecoder::cOptions {
public:
  virtual ~cPlayerOptions() = default;

  string getString() const { return "sub motion fileName"; }

  // vars
  bool mShowSubtitle = false;
  bool mShowMotionVectors = false;
  string mFileName;
  };
//}}}
//{{{
class cFilePlayer {
public:
  cFilePlayer (string fileName, cPlayerOptions* options) :
    mFileName(cFileUtils::resolve (fileName)), mOptions(options) {}
  //{{{
  virtual ~cFilePlayer() {
    mAudioPesMap.clear();
    mSubtitlePesMap.clear();
    mVideoPesMap.clear();
    mVideoGopMap.clear();
    }
  //}}}

  string getFileName() const { return mFileName; }

  cTransportStream::cService* getService() { return mService; }

  cAudioRender* getAudioRender() { return mAudioRender; }
  cVideoRender* getVideoRender() { return mVideoRender; }
  cSubtitleRender* getSubtitleRender() { return mSubtitleRender; }

  int64_t getFirstVideoPts() const { return mVideoPesMap.empty() ? -1 : mVideoPesMap.begin()->first; }
  int64_t getFirstAudioPts() const { return mAudioPesMap.empty() ? -1 : mAudioPesMap.begin()->first; }

  //{{{
  void start() {
    analyse();
    audioLoader();
    videoLoader();
    }
  //}}}
  //{{{
  void togglePlay() {
    if (mAudioRender)
      mAudioRender->togglePlay();
    }
  //}}}
  //{{{
  void skip (int64_t skipPts) {
    if (mAudioRender)
      mAudioRender->skip (skipPts);
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

    void addPes (sPes pes) {
      mPesVector.push_back (pes);
      }

    vector <sPes> mPesVector;
    };
  //}}}

  //{{{
  bool analyse() {
  // launch analyser thread

    FILE* file = fopen (mFileName.c_str(), "rb");
    if (!file) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("cFilePlayer::analyse to open {}", mFileName));
      return false;
      }
      //}}}

    cTransportStream* transportStream = new cTransportStream (
      {"anal", 0, {}, {}}, mOptions,
      [&](cTransportStream::cService& service) noexcept { mService = &service; }, // newService lambda
      //{{{  pes lambda
      [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo, bool skip) noexcept {

        (void)skip;
        mPesBytes += pidInfo.getBufSize();
        uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
        memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());

        // add pes to map
        if (pidInfo.getPid() == service.getVideoPid()) {
          mNumVideoPes++;
          char frameType = cDvbUtils::getFrameType (buffer, pidInfo.getBufSize(), true);

          if (frameType == 'I')
            mVideoGopMap.emplace (pidInfo.getPts(),
                                  cGop(sPes(buffer, pidInfo.getBufSize(),
                                            pidInfo.getPts(), pidInfo.getDts(), frameType)));
          else if (!mVideoGopMap.empty())
            mVideoGopMap.rbegin()->second.addPes (sPes(buffer, pidInfo.getBufSize(),
                                                       pidInfo.getPts(), pidInfo.getDts(), frameType));
          else
            mVideoPesMap.emplace (pidInfo.getDts(),
                                  sPes(buffer, pidInfo.getBufSize(),
                                       pidInfo.getPts(), pidInfo.getDts(), frameType));

          cLog::log (LOGINFO, fmt::format ("{}:{:2d} dts:{} {} size:{:6d}",
                                           frameType,
                                           mVideoGopMap.empty() ? 0 : mVideoGopMap.rbegin()->second.mPesVector.size(),
                                           utils::getFullPtsString (pidInfo.getDts()),
                                           utils::getFullPtsString (pidInfo.getPts()),
                                           pidInfo.getBufSize()
                                           ));

          }
        else if (pidInfo.getPid() == service.getAudioPid()) {
          //cLog::log (LOGINFO, fmt::format ("A {:6d} {} {}",
          //                                 pidInfo.getBufSize(),
          //                                 utils::getFullPtsString (pidInfo.getDts()),
          //                                 utils::getFullPtsString (pidInfo.getPts())));

          unique_lock<shared_mutex> lock (mAudioMutex);
          mAudioPesMap.emplace (pidInfo.getPts(),
                                sPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
          }
        else if ((pidInfo.getPid() == service.getSubtitlePid()) && pidInfo.getBufSize())
          mSubtitlePesMap.emplace (pidInfo.getPts(),
                                   sPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
        }
      //}}}
      );
    if (!transportStream) {
      //{{{  error, return
      cLog::log (LOGERROR, "cFilePlayer::analyser ts create failed");
      return false;
      }
      //}}}

    thread ([=]() {
      cLog::setThreadName ("anal");
      size_t chunkSize = 188 * 256;
      uint8_t* chunk = new uint8_t[chunkSize];
      auto now = chrono::system_clock::now();
      int64_t streamPos = 0;
      while (true) {
        size_t bytesRead = fread (chunk, 1, chunkSize, file);
        if (bytesRead > 0)
          streamPos += transportStream->demux (chunk, bytesRead, streamPos, false);
        else
          break;
        }
      delete[] chunk;
      fclose (file);
      delete transportStream;

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
      //{{{  log totals
      cLog::log (LOGINFO, fmt::format ("size:{:8d}:{:8d} took {}ms",
        mPesBytes, mFileSize,
        chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count()));
      cLog::log (LOGINFO, fmt::format ("- vid:{:6d} {} to {} gop:{} nonGopPes:{}",
                                       mNumVideoPes,
                                       utils::getFullPtsString (mVideoPesMap.begin()->first),
                                       utils::getFullPtsString (mVideoPesMap.rbegin()->first),
                                       mVideoGopMap.size(), mVideoPesMap.size()));
      cLog::log (LOGINFO, fmt::format ("- aud:{:6d} {} to {}",
                                       mAudioPesMap.size(),
                                       utils::getFullPtsString (mAudioPesMap.begin()->first),
                                       utils::getFullPtsString (mAudioPesMap.rbegin()->first)));
      cLog::log (LOGINFO, fmt::format ("- sub:{:6d} {} to {}",
                                       mSubtitlePesMap.size(),
                                       utils::getFullPtsString (mSubtitlePesMap.begin()->first),
                                       utils::getFullPtsString (mSubtitlePesMap.rbegin()->first)));
      cLog::log (LOGERROR, "exit");
      //}}}
      }).detach();

    return true;
    }
  //}}}
  //{{{
  void audioLoader() {
  // launch audioLoader thread

    thread ([=]() {
      cLog::setThreadName ("audL");

      while (mAudioPesMap.begin() == mAudioPesMap.end()) {
        cLog::log (LOGINFO, fmt::format ("audioLoader::start wait"));
        this_thread::sleep_for (100ms);
        }

      mAudioRender = new cAudioRender (
        false, 50, "aud", mService->getAudioStreamTypeId(), mService->getAudioPid(), mOptions);

      auto it = mAudioPesMap.begin();
      mAudioRender->decodePes (it->second.mData, it->second.mSize, it->second.mPts, it->second.mDts);
      ++it;

      //unique_lock<shared_mutex> lock (mAudioMutex);
      while (it != mAudioPesMap.end()) {
        int64_t diff = it->first - mAudioRender->getPlayer()->getPts();
        //cLog::log (LOGINFO, fmt::format ("{:5d} {} - {}",
        //                                 diff,
        //                                 utils::getFullPtsString (it->first),
        //                                 utils::getFullPtsString (mAudioRender->getPlayer()->getPts())));
        mAudioRender->decodePes (it->second.mData, it->second.mSize, it->second.mPts, it->second.mDts);
        ++it;
        while (mAudioRender->throttle (mAudioRender->getPlayer()->getPts()))
          this_thread::sleep_for (1ms);
        }

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
  //}}}
  //{{{
  void videoLoader() {
  // launch videoLoader thread

    thread ([=]() {
      cLog::setThreadName ("vidL");

      while ((mVideoPesMap.begin() == mVideoPesMap.end()) || !mAudioRender || !mAudioRender->getPlayer()) {
        cLog::log (LOGINFO, fmt::format ("videoLoader::start wait"));
        this_thread::sleep_for (100ms);
        }

      mVideoRender = new cVideoRender (
        false, 50, "vid", mService->getVideoStreamTypeId(), mService->getVideoPid(), mOptions);

      auto gopIt = mVideoGopMap.begin();
      while (gopIt != mVideoGopMap.end()) {
        int64_t gopDiff = gopIt->first - mAudioRender->getPlayer()->getPts();
        cLog::log (LOGINFO, fmt::format (" - play Gop {:5d} {} - {}",
                                         gopDiff,
                                         utils::getFullPtsString (gopIt->first),
                                         utils::getFullPtsString (mAudioRender->getPlayer()->getPts())));

        auto& pesVector = gopIt->second.mPesVector;
        auto it = pesVector.begin();
        while (it != pesVector.end()) {
          int64_t diff = it->mPts - mAudioRender->getPlayer()->getPts();
          cLog::log (LOGINFO, fmt::format ("   - play Pes {:5d} {} - {}",
                                           diff,
                                           utils::getFullPtsString (it->mPts),
                                           utils::getFullPtsString (mAudioRender->getPlayer()->getPts())));
          mVideoRender->decodePes (it->mData, it->mSize, it->mPts, it->mDts);
          ++it;

          while (mVideoRender->throttle (mAudioRender->getPlayer()->getPts()))
            this_thread::sleep_for (1ms);
          }

        ++gopIt;
        }

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
  //}}}
  //{{{
  void subtitleLoader() {
  // launch subtitleLoader thread

    thread ([=]() {
      cLog::setThreadName ("subL");

      while ((mSubtitlePesMap.begin() == mSubtitlePesMap.end()) || !mAudioRender || !mAudioRender->getPlayer()) {
        cLog::log (LOGINFO, fmt::format ("subtitleLoader::start wait"));
        this_thread::sleep_for (100ms);
        }

      mSubtitleRender = new cSubtitleRender (
        false, 0, "sub", mService->getSubtitleStreamTypeId(), mService->getSubtitlePid(), mOptions);

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
  //}}}

  string mFileName;
  cPlayerOptions* mOptions;

  size_t mFileSize = 0;
  cTransportStream::cService* mService = nullptr;

  int64_t mPesBytes = 0;
  int64_t mLastI = -1;

  // pes
  shared_mutex mAudioMutex;
  map <int64_t,sPes> mAudioPesMap;

  int64_t mNumVideoPes = 0;
  map <int64_t,sPes> mVideoPesMap;
  map <int64_t,cGop> mVideoGopMap;

  map <int64_t,sPes> mSubtitlePesMap;

  // render
  cVideoRender* mVideoRender = nullptr;
  cAudioRender* mAudioRender = nullptr;
  cSubtitleRender* mSubtitleRender = nullptr;
  };
//}}}
//{{{
class cPlayerApp : public cApp {
public:
  cPlayerApp (cPlayerOptions* options, iUI* ui) : cApp ("Player", options, ui), mOptions(options) {}
  //{{{
  virtual ~cPlayerApp() {
    // delete filePlayes
    mFilePlayers.clear();
    }
  //}}}

  //{{{
  void addFile (const string& fileName, cPlayerOptions* options) {

    cFilePlayer* filePlayer = new cFilePlayer (fileName, options);

    mFilePlayers.push_back (filePlayer);
    filePlayer->start();
    }
  //}}}

  cPlayerOptions* getOptions() { return mOptions; }
  cFilePlayer* getFilePlayer() { return mFilePlayers.front(); }

  // actions
  void toggleShowSubtitle() { mOptions->mShowSubtitle = !mOptions->mShowSubtitle; }
  void toggleShowMotionVectors() { mOptions->mShowMotionVectors = !mOptions->mShowMotionVectors; }

  // play
  //{{{
  void togglePlay() {
    mFilePlayers.front()->togglePlay();
    }
  //}}}
  //{{{
  void skipPlay (int64_t skipPts) {
    cLog::log (LOGINFO, fmt::format ("skip:{}", skipPts));
    mFilePlayers.front()->skip (skipPts);
    }
  //}}}

  //{{{
  void drop (const vector<string>& dropItems) {
  // drop fileNames

    for (auto& item : dropItems) {
      cLog::log (LOGINFO, fmt::format ("cPlayerApp::drop {}", item));
      addFile (item, mOptions);
      }
    }
  //}}}

private:
  cPlayerOptions* mOptions;
  vector<cFilePlayer*> mFilePlayers;
  };
//}}}
//{{{
class cPlayerUI : public cApp::iUI {
public:
  virtual ~cPlayerUI() = default;

  //{{{
  void draw (cApp& app) {

    ImGui::SetKeyboardFocusHere();

    app.getGraphics().clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                              (int32_t)ImGui::GetIO().DisplaySize.y});

    // draw UI
    ImGui::SetNextWindowPos ({0.f,0.f});
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    ImGui::Begin ("player", nullptr, ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                     ImGuiWindowFlags_NoBackground);

    cPlayerApp& playerApp = (cPlayerApp&)app;

    // draw multiView piccies
    mMultiView.draw (playerApp);

    // draw menu
    ImGui::SetCursorPos ({0.f, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeight() * 1.5f});
    ImGui::BeginChild ("menu", {0.f, ImGui::GetTextLineHeight() * 1.5f},
                       ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoBackground);

    ImGui::SetCursorPos ({0.f,0.f});
    //{{{  draw subtitle button
    ImGui::SameLine();
    if (toggleButton ("sub", playerApp.getOptions()->mShowSubtitle))
      playerApp.toggleShowSubtitle();
    //}}}
    if (playerApp.getOptions()->mHasMotionVectors) {
      //{{{  draw motionVectors button
      ImGui::SameLine();
      if (toggleButton ("motion", playerApp.getOptions()->mShowMotionVectors))
        playerApp.toggleShowMotionVectors();
      }
      //}}}
    if (playerApp.getPlatform().hasFullScreen()) {
      //{{{  draw fullScreen button
      ImGui::SameLine();
      if (toggleButton ("full", playerApp.getPlatform().getFullScreen()))
        playerApp.getPlatform().toggleFullScreen();
      }
      //}}}
    if (playerApp.getPlatform().hasVsync()) {
      //{{{  draw vsync button
      ImGui::SameLine();
      if (toggleButton ("vsync", playerApp.getPlatform().getVsync()))
        playerApp.getPlatform().toggleVsync();
      }
      //}}}
    //{{{  draw frameRate info
    ImGui::SameLine();
    ImGui::TextUnformatted (fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
    //}}}

    ImGui::EndChild();
    ImGui::End();

    keyboard (playerApp);
    }
  //}}}

private:
  //{{{
  class cFramesView {
  public:
    void draw (cAudioRender* audioRender, cVideoRender* videoRender, int64_t playerPts, ImVec2 pos) {

     float ptsScale = 6.f / videoRender->getPtsDuration();
      if (videoRender) {
        // lock video during iterate
        shared_lock<shared_mutex> lock (videoRender->getSharedMutex());
        for (auto& frame : videoRender->getFramesMap()) {
          //{{{  draw video frames
          float posL = pos.x + (frame.second->getPts() - playerPts) * ptsScale;
          float posR = pos.x + ((frame.second->getPts() - playerPts + frame.second->getPtsDuration()) * ptsScale) - 1.f;

          // pesSize I white / P purple / B blue - ABGR color
          cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame.second);
          ImGui::GetWindowDrawList()->AddRectFilled (
            { posL, pos.y - addValue ((float)videoFrame->getPesSize(), mMaxPesSize,
                                         2.f * ImGui::GetTextLineHeight()) },
            { posR, pos.y },
            (videoFrame->mFrameType == 'I') ?
              0xffffffff : (videoFrame->mFrameType == 'P') ?
                0xffFF40ff : 0xffFF4040);
          }
          //}}}
        }

      if (audioRender) {
        // lock audio during iterate
        shared_lock<shared_mutex> lock (audioRender->getSharedMutex());
        for (auto& frame : audioRender->getFramesMap()) {
          //{{{  draw audio frames
          float posL = pos.x + (frame.second->getPts() - playerPts) * ptsScale;
          float posR = pos.x + ((frame.second->getPts() - playerPts + frame.second->getPtsDuration()) * ptsScale) - 1.f;

          cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame.second);
          ImGui::GetWindowDrawList()->AddRectFilled (
            { posL, pos.y - addValue (audioFrame->getSimplePower(), mMaxPower,
                                      2.f * ImGui::GetTextLineHeight()) },
            { posR, pos.y },
            0x8000ff00);
          }
          //}}}
        }

      // draw playPts centre bar
      ImGui::GetWindowDrawList()->AddRectFilled (
        { pos.x-1.f, pos.y - (2.f * ImGui::GetTextLineHeight()) },
        { pos.x+1.f, pos.y },
        0xffffffff);
      }

  private:
    //{{{
    float addValue (float value, float& maxValue, float scale) {

      if (value > maxValue)
        maxValue = value;
      return scale * value / maxValue;
      }
    //}}}
    //{{{
    void agc (float& maxValue, float& maxDisplayValue, float revert, float minValue) {

      // slowly adjust scale to displayMaxValue
      if (maxDisplayValue < maxValue)
        maxValue += (maxDisplayValue - maxValue) / revert;

      if (maxValue < minValue)
        maxValue = minValue;

      // reset displayed max value
      maxDisplayValue = 0.f;
      }
    //}}}

    //  vars
    const float mPixelsPerAudioChannel = 6.f;
    float mMaxPower = 0.f;
    float mMaxPesSize = 0.f;
    };
  //}}}
  //{{{
  class cAudioMeterView {
  public:
    void draw (cAudioRender* audioRender, int64_t playerPts, ImVec2 pos) {
      cAudioFrame* audioFrame = audioRender->getAudioFrameAtPts (playerPts);
      if (audioFrame) {
        size_t drawChannels = audioFrame->getNumChannels();
        bool audio51 = (audioFrame->getNumChannels() == 6);
        array <size_t, 6> channelOrder;
        if (audio51) {
          channelOrder = { 4, 0, 2, 1, 5, 3 };
          drawChannels--;
          }
        else
          channelOrder = { 0, 1, 2, 3, 4, 5 };

        // draw channels
        float x = pos.x - (6.f * drawChannels);
        pos.y += 1.f;
        float height = 8.f * ImGui::GetTextLineHeight();
        for (size_t i = 0; i < drawChannels; i++) {
          ImGui::GetWindowDrawList()->AddRectFilled (
            {x, pos.y - (audioFrame->mPowerValues[channelOrder[i]] * height)},
            {x + 6.f - 1.f, pos.y},
            0xff00ff00);
          x += 6.f;
          }

        // draw 5.1 woofer red
        if (audio51)
          ImGui::GetWindowDrawList()->AddRectFilled (
            {pos.x - (5 * 6.f), pos.y - (audioFrame->mPowerValues[channelOrder[5]] * height)},
            {pos.x - 1.f, pos.y},
            0x800000ff);
        }
      }
    };
  //}}}
  //{{{
  class cView {
  public:
    cView (cTransportStream::cService& service) : mService(service) {}
    //{{{
    ~cView() {
      delete mVideoQuad;

      for (size_t i = 0; i < kMaxSubtitleLines; i++) {
        delete mSubtitleQuads[i];
        delete mSubtitleTextures[i];
        }
      }
    //}}}

    bool getSelectedFull() const { return mSelect == eSelectedFull; }
    //{{{
    void unselect() {
      if (mSelect == eSelected)
        mSelect = eUnselected;
      }
    //}}}

    //{{{
    bool draw (cPlayerApp& playerApp, bool selectFull, size_t viewIndex, size_t numViews,
               cTextureShader* videoShader, cTextureShader* subtitleShader) {
    // return true if hit

      bool result = false;

      float layoutScale;
      cVec2 layoutPos = getLayout (viewIndex, numViews, layoutScale);
      float viewportWidth = ImGui::GetWindowWidth();
      float viewportHeight = ImGui::GetWindowHeight();
      mSize = {layoutScale * viewportWidth, layoutScale * viewportHeight};
      mTL = {(layoutPos.x * viewportWidth) - mSize.x*0.5f,
             (layoutPos.y * viewportHeight) - mSize.y*0.5f};
      mBR = {(layoutPos.x * viewportWidth) + mSize.x*0.5f,
             (layoutPos.y * viewportHeight) + mSize.y*0.5f};

      ImGui::SetCursorPos (mTL);
      ImGui::BeginChild (fmt::format ("view##{}", mService.getSid()).c_str(), mSize,
                         ImGuiChildFlags_None,
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoBackground);
      //{{{  draw select rect
      bool hover = ImGui::IsMouseHoveringRect (mTL, mBR);
      if ((hover || (mSelect != eUnselected)) && (mSelect != eSelectedFull))
        ImGui::GetWindowDrawList()->AddRect (mTL, mBR, hover ? 0xff20ffff : 0xff20ff20, 4.f, 0, 4.f);
      //}}}

      cAudioRender* audioRender = playerApp.getFilePlayer()->getAudioRender();
      if (audioRender) {
        //  get audio playPts
        int64_t playPts = audioRender->getPts();
        if (audioRender->getPlayer()) {
          playPts = audioRender->getPlayer()->getPts();
          audioRender->getPlayer()->setMute (false);
          }

        cVideoRender* videoRender = playerApp.getFilePlayer()->getVideoRender();
        if (videoRender) {
          cVideoFrame* videoFrame = videoRender->getVideoFrameAtOrAfterPts (playPts);
          if (videoFrame) {
            //{{{
            // draw video
            cMat4x4 model = cMat4x4();
            model.setTranslate ({(layoutPos.x - (0.5f * layoutScale)) * viewportWidth,
                                 ((1.f-layoutPos.y) - (0.5f * layoutScale)) * viewportHeight});
            model.size ({layoutScale * viewportWidth / videoFrame->getWidth(),
                         layoutScale * viewportHeight / videoFrame->getHeight()});
            cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);
            videoShader->use();
            videoShader->setModelProjection (model, projection);

            // texture
            cTexture& texture = videoFrame->getTexture (playerApp.getGraphics());
            texture.setSource();

            // ensure quad is created
            if (!mVideoQuad)
              mVideoQuad = playerApp.getGraphics().createQuad (videoFrame->getSize());

            // draw quad
            mVideoQuad->draw();

            //if (playerApp.getOptions()->mShowSubtitle) {
              //{{{  draw subtitles
              //cSubtitleRender& subtitleRender =
                //dynamic_cast<cSubtitleRender&> (mService.getStream (cRenderStream::eSubtitle).getRender());

              //subtitleShader->use();
              //for (size_t line = 0; line < subtitleRender.getNumLines(); line++) {
                //cSubtitleImage& subtitleImage = subtitleRender.getImage (line);
                //if (!mSubtitleTextures[line])
                  //mSubtitleTextures[line] = playerApp.getGraphics().createTexture (cTexture::eRgba, subtitleImage.getSize());
                //mSubtitleTextures[line]->setSource();

                //// update subtitle texture if image dirty
                //if (subtitleImage.isDirty())
                  //mSubtitleTextures[line]->setPixels (subtitleImage.getPixels(), nullptr);
                //subtitleImage.setDirty (false);

                //float xpos = (float)subtitleImage.getXpos() / videoFrame->getWidth();
                //float ypos = (float)(videoFrame->getHeight() - subtitleImage.getYpos()) / videoFrame->getHeight();
                //model.setTranslate ({(layoutPos.x + ((xpos - 0.5f) * layoutScale)) * viewportWidth,
                                     //((1.0f - layoutPos.y) + ((ypos - 0.5f) * layoutScale)) * viewportHeight});
                //subtitleShader->setModelProjection (model, projection);

                //// ensure quad is created (assumes same size) and draw
                //if (!mSubtitleQuads[line])
                  //mSubtitleQuads[line] = playerApp.getGraphics().createQuad (mSubtitleTextures[line]->getSize());
                //mSubtitleQuads[line]->draw();
                //}
              //}
              //}}}

            //if (playerApp.getOptions()->mShowMotionVectors && (mSelect == eSelectedFull)) {
              //{{{  draw motion vectors
              //size_t numMotionVectors;
              //AVMotionVector* mv = (AVMotionVector*)(videoFrame->getMotionVectors (numMotionVectors));
              //if (numMotionVectors) {
                //for (size_t i = 0; i < numMotionVectors; i++) {
                  //ImGui::GetWindowDrawList()->AddLine (
                    //mTL + ImVec2(mv->src_x * viewportWidth / videoFrame->getWidth(),
                                 //mv->src_y * viewportHeight / videoFrame->getHeight()),
                    //mTL + ImVec2((mv->src_x + (mv->motion_x / mv->motion_scale)) * viewportWidth / videoFrame->getWidth(),
                                  //(mv->src_y + (mv->motion_y / mv->motion_scale)) * viewportHeight / videoFrame->getHeight()),
                    //mv->source > 0 ? 0xc0c0c0c0 : 0xc000c0c0, 1.f);
                  //mv++;
                  //}
                //}
              //}
              //}}}
            }
            //}}}

          //if (mSelect == eSelectedFull) // draw framesView
          mFramesView.draw (audioRender, videoRender, playPts,
                            ImVec2((mTL.x + mBR.x)/2.f, mBR.y - ImGui::GetTextLineHeight()*0.25f));
          }

        // largeFont
        ImGui::PushFont (playerApp.getLargeFont());
        //{{{  draw title topLeft
        string title = "title";

        ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
        ImGui::SetCursorPos (pos);
        ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
        ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
        ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());

        pos.y += ImGui::GetTextLineHeight() * 1.5f;
        //}}}
        if (audioRender) {
          //{{{  draw playPts bottomRight
          string ptsFromStartString = utils::getPtsString (audioRender->getPts());

          pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
          ImGui::SetCursorPos (pos);
          ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsFromStartString.c_str());
          ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
          ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsFromStartString.c_str());
          }
          //}}}
        ImGui::PopFont();

        if (audioRender)
          mAudioMeterView.draw (audioRender, playPts,
                                ImVec2(mBR.x - ImGui::GetTextLineHeight()*0.5f,
                                       mBR.y - ImGui::GetTextLineHeight()*0.25f));
        }

      ImVec2 viewSubSize = mSize - ImVec2(0.f,
          ImGui::GetTextLineHeight() * ((layoutPos.y + (layoutScale/2.f) >= 0.99f) ? 3.f : 1.5f));

      // invisbleButton over view sub area
      ImGui::SetCursorPos ({0.f,0.f});
      if (ImGui::InvisibleButton (fmt::format ("viewBox##{}", mService.getSid()).c_str(), viewSubSize)) {
        //{{{  hit view, select action
        if (mSelect == eUnselected)
          mSelect = eSelected;
        else if (mSelect == eSelected)
          mSelect = eSelectedFull;
        else if (mSelect == eSelectedFull)
          mSelect = eSelected;

        result = true;
        }
        //}}}

      ImGui::EndChild();
      return result;
      }
    //}}}

  private:
    static inline const size_t kMaxSubtitleLines = 4;
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
    cTransportStream::cService& mService;
    eSelect mSelect = eUnselected;

    // video
    ImVec2 mSize;
    ImVec2 mTL;
    ImVec2 mBR;
    cQuad* mVideoQuad = nullptr;

    // graphics
    cFramesView mFramesView;
    cAudioMeterView mAudioMeterView;

    // subtitle
    array <cQuad*, kMaxSubtitleLines> mSubtitleQuads = { nullptr };
    array <cTexture*, kMaxSubtitleLines> mSubtitleTextures = { nullptr };
    };
  //}}}
  //{{{
  class cMultiView {
  public:
    //{{{
    void draw (cPlayerApp& playerApp) {

      // create shaders, firsttime we see graphics interface
      if (!mVideoShader)
        mVideoShader = playerApp.getGraphics().createTextureShader (cTexture::eYuv420);
      if (!mSubtitleShader)
        mSubtitleShader = playerApp.getGraphics().createTextureShader (cTexture::eRgba);

      // update viewMap from filePlayers
      cFilePlayer* filePlayer = playerApp.getFilePlayer();

      auto it = mViewMap.find (filePlayer->getService()->getSid());
      if (it == mViewMap.end()) {
        mViewMap.emplace (filePlayer->getService()->getSid(), cView (*filePlayer->getService()));
        cLog::log (LOGINFO, fmt::format ("cMultiView::draw add view {}", filePlayer->getService()->getSid()));
        }

      // any view selectedFull ?
      bool selectedFull = false;
      for (auto& view : mViewMap)
        selectedFull |= (view.second.getSelectedFull());

      // draw views
      bool viewHit = false;
      size_t viewHitIndex = 0;
      size_t viewIndex = 0;
      for (auto& view : mViewMap) {
        if (!selectedFull || view.second.getSelectedFull())
          if (view.second.draw (playerApp, selectedFull, viewIndex, selectedFull ? 1 : mViewMap.size(),
                                mVideoShader, mSubtitleShader)) {
            // view hit
            viewHit = true;
            viewHitIndex = viewIndex;
            }
        viewIndex++;
        }

      // unselect other views
      if (viewHit) {
        size_t unselectViewIndex = 0;
        for (auto& view : mViewMap) {
          if (unselectViewIndex != viewHitIndex)
            view.second.unselect();
          unselectViewIndex++;
          }
        }
      }
    //}}}

    //{{{
    void hitEnter() {
      cLog::log (LOGINFO, fmt::format ("multiView hitEnter"));
      }
    //}}}
    //{{{
    void hitSpace() {
      cLog::log (LOGINFO, fmt::format ("multiView hitSpace"));
      }
    //}}}
    //{{{
    void hitLeft() {
      cLog::log (LOGINFO, fmt::format ("multiView hitLeft"));
      }
    //}}}
    //{{{
    void hitRight() {
      cLog::log (LOGINFO, fmt::format ("multiView hitRight"));
      }
    //}}}
    //{{{
    void hitUp() {
      cLog::log (LOGINFO, fmt::format ("multiView hitUp"));
      }
    //}}}
    //{{{
    void hitDown() {
      cLog::log (LOGINFO, fmt::format ("multiView hitDown"));
      }
    //}}}

  private:
    map <uint16_t, cView> mViewMap;
    cTextureShader* mVideoShader = nullptr;
    cTextureShader* mSubtitleShader = nullptr;
    };
  //}}}

  //{{{
  void hitSpace (cPlayerApp& playerApp) {
    playerApp.togglePlay();
    }
  //}}}
  //{{{
  void hitControlLeft (cPlayerApp& playerApp) {
    playerApp.skipPlay (-90000 / 25);
    }
  //}}}
  //{{{
  void hitControlRight (cPlayerApp& playerApp) {
    playerApp.skipPlay (90000 / 25);
    }
  //}}}
  //{{{
  void hitShiftLeft (cPlayerApp& playerApp) {
    playerApp.skipPlay (-(90000 * 10) / 25);
    }
  //}}}
  //{{{
  void hitShiftRight (cPlayerApp& playerApp) {
    playerApp.skipPlay ((90000 * 10) / 25);
    }
  //}}}
  //{{{
  void hitLeft (cPlayerApp& playerApp) {
    playerApp.skipPlay (-90000);
    }
  //}}}
  //{{{
  void hitRight (cPlayerApp& playerApp) {
    playerApp.skipPlay (90000);
    }
  //}}}
  //{{{
  void hitUp (cPlayerApp& playerApp) {
    playerApp.skipPlay (-900000);
    }
  //}}}
  //{{{
  void hitDown (cPlayerApp& playerApp) {
    playerApp.skipPlay (900000);
    }
  //}}}
  //{{{
  void keyboard (cPlayerApp& playerApp) {

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
      { false, true,   false, ImGuiKey_LeftArrow,  [this,&playerApp]{ hitControlLeft (playerApp); }},
      { false, false,  true,  ImGuiKey_LeftArrow,  [this,&playerApp]{ hitShiftLeft (playerApp); }},
      { false, false,  false, ImGuiKey_LeftArrow,  [this,&playerApp]{ hitLeft (playerApp); }},
      { false, false,  true,  ImGuiKey_RightArrow, [this,&playerApp]{ hitShiftRight (playerApp); }},
      { false, true,   false, ImGuiKey_RightArrow, [this,&playerApp]{ hitControlRight (playerApp); }},
      { false, false,  false, ImGuiKey_RightArrow, [this,&playerApp]{ hitRight (playerApp); }},
      { false, false,  false, ImGuiKey_UpArrow,    [this,&playerApp]{ hitUp (playerApp); }},
      { false, false,  false, ImGuiKey_DownArrow,  [this,&playerApp]{ hitDown (playerApp); }},
      { false, false,  false, ImGuiKey_Space,      [this,&playerApp]{ hitSpace (playerApp); }},
      { false, false,  false, ImGuiKey_Enter,      [this,&playerApp]{ mMultiView.hitEnter(); }},
      { false, false,  false, ImGuiKey_F,          [this,&playerApp]{ playerApp.getPlatform().toggleFullScreen(); }},
      { false, false,  false, ImGuiKey_S,          [this,&playerApp]{ playerApp.toggleShowSubtitle(); }},
      { false, false,  false, ImGuiKey_L,          [this,&playerApp]{ playerApp.toggleShowMotionVectors(); }},
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
  cMultiView mMultiView;
  };
//}}}

// main
int main (int numArgs, char* args[]) {

  // options
  cPlayerOptions* options = new cPlayerOptions();
  //{{{  parse commandLine options
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) // found cApp option
      ;
    else if (param == "sub")
      options->mShowSubtitle = true;
    else if (param == "motion")
      options->mHasMotionVectors = true;
    else
      options->mFileName = param;
    }
  //}}}

  // log
  cLog::init (options->mLogLevel);

  // launch
  cPlayerApp playerApp (options, new cPlayerUI());
  playerApp.addFile (options->mFileName, options);
  playerApp.mainUILoop();

  return EXIT_SUCCESS;
  }

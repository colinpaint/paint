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
#include "../decoders/cAudioFrame.h"
#include "../decoders/cVideoFrame.h"
#include "../decoders/cSubtitleFrame.h"
#include "../decoders/cSubtitleImage.h"
#include "../decoders/cVideoRender.h"
#include "../decoders/cAudioRender.h"
#include "../decoders/cSubtitleRender.h"
#include "../decoders/cFFmpegVideoDecoder.h"
#include "../decoders/cAudioPlayer.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/myImgui.h"

using namespace std;
using namespace utils;
//}}}
constexpr bool kAnalDebug = false;
constexpr bool kAnalTotals = true;
constexpr bool kAudioLoadDebug = true;
constexpr bool kVideoLoadDebug = false;
constexpr bool kSubtitleLoadDebug = false;

namespace {
  //{{{
  class cPlayerOptions : public cApp::cOptions, public cTransportStream::cOptions {
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
      mGopMap.clear();
      }
    //}}}

    string getFileName() const { return mFileName; }

    cTransportStream::cService* getService() { return mService; }

    cAudioRender* getAudioRender() { return mAudioRender; }
    cVideoRender* getVideoRender() { return mVideoRender; }
    cSubtitleRender* getSubtitleRender() { return mSubtitleRender; }

    // audioPlayer
    cAudioPlayer* getAudioPlayer() { return mAudioRender ? mAudioRender->getAudioPlayer() : nullptr; }
    //{{{
    void togglePlay() {
      if (getAudioPlayer())
        getAudioPlayer()->togglePlay();
      }
    //}}}
    //{{{
    void skip (int64_t skipPts) {
      if (getAudioPlayer())
        getAudioPlayer()->skipPts (skipPts);
      }
    //}}}

    //{{{
    void start() {
      analyseThread();
      audioLoaderThread();
      videoLoaderThread();
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

        cTransportStream transportStream (
          {"anal", 0, {}, {}}, mOptions,
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
              if (kAnalDebug)
                //{{{  debug
                cLog::log (LOGINFO, fmt::format ("V {}:{:2d} dts:{} pts:{} size:{}",
                                                 frameType,
                                                 mGopMap.empty() ? 0 : mGopMap.rbegin()->second.mPesVector.size(),
                                                 getFullPtsString (pidInfo.getDts()),
                                                 getFullPtsString (pidInfo.getPts()),
                                                 pidInfo.getBufSize()
                                                 ));

                //}}}
              }

            else if (pidInfo.getPid() == service.getAudioPid()) {
              // add audio pes, take copy of pes
              uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
              memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());
              if (kAnalDebug)
                cLog::log (LOGINFO, fmt::format ("A pts:{} size:{}",
                                                 getFullPtsString (pidInfo.getPts()), pidInfo.getBufSize()));

              unique_lock<shared_mutex> lock (mAudioMutex);
              mAudioPesMap.emplace (pidInfo.getPts(),
                                    sPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
              }
            else if (pidInfo.getPid() == service.getSubtitlePid()) {
              if (pidInfo.getBufSize()) {
                // add subtitle pes, take copy of pes
                uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
                memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());
                if (kAnalDebug)
                  cLog::log (LOGINFO, fmt::format ("S pts:{} size:{}",
                                                 getFullPtsString (pidInfo.getPts()), pidInfo.getBufSize()));

                mSubtitlePesMap.emplace (pidInfo.getPts(),
                                         sPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
                }
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
            transportStream.demux (chunk, bytesRead);
          else
            break;
          }

        delete[] chunk;
        fclose (file);

        if (kAnalTotals) {
          //{{{  report totals
          cLog::log (LOGINFO, fmt::format ("{}m took {}ms",
            mFileSize/1000000,
            chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count()));

          size_t numVidPes = 0;
          for (auto& gop : mGopMap)
            numVidPes += gop.second.mPesVector.size();

          cLog::log (LOGINFO, fmt::format ("- vid:{:6d} {} to {} gop:{}",
                                           numVidPes,
                                           getFullPtsString (mGopMap.begin()->second.mPesVector.front().mPts),
                                           getFullPtsString (mGopMap.rbegin()->second.mPesVector.front().mPts),
                                           mGopMap.size()));

          cLog::log (LOGINFO, fmt::format ("- aud:{:6d} {} to {}",
                                           mAudioPesMap.size(),
                                           getFullPtsString (mAudioPesMap.begin()->first),
                                           getFullPtsString (mAudioPesMap.rbegin()->first)));

          cLog::log (LOGINFO, fmt::format ("- sub:{:6d} {} to {}",
                                           mSubtitlePesMap.size(),
                                           getFullPtsString (mSubtitlePesMap.begin()->first),
                                           getFullPtsString (mSubtitlePesMap.rbegin()->first)));
          }
          //}}}

        cLog::log (LOGERROR, "exit");
        }).detach();

      return true;
      }
    //}}}
    //{{{
    void audioLoaderThread() {
    // launch audioLoader thread

      thread ([=]() {
        cLog::setThreadName ("audL");

        while (mAudioPesMap.begin() == mAudioPesMap.end()) {
          // wait for analyse to see some audioPes
          if (kAudioLoadDebug)
            cLog::log (LOGINFO, fmt::format ("audioLoader start wait"));
          this_thread::sleep_for (100ms);
          }

        // creates audioPlayer on first audio pes
        mAudioRender = new cAudioRender (false, 100, true, true,
                                         "aud", mService->getAudioStreamTypeId(), mService->getAudioPid());

        //unique_lock<shared_mutex> lock (mAudioMutex);
        auto pesIt = mAudioPesMap.begin();
        if (kAudioLoadDebug)
          cLog::log (LOGINFO, fmt::format ("A pts:{}", getFullPtsString (pesIt->first)));
        mAudioRender->decodePes (pesIt->second.mData, pesIt->second.mSize, pesIt->second.mPts, pesIt->second.mDts);
        mLastLoadedAudioPts = pesIt->first;
        ++pesIt;

        while (pesIt != mAudioPesMap.end()) {
          if (kAudioLoadDebug)
            cLog::log (LOGINFO, fmt::format ("A pts:{} player:{}",
                                             getFullPtsString (pesIt->first),
                                             getFullPtsString (getAudioPlayer()->getPts())));
          mAudioRender->decodePes (pesIt->second.mData, pesIt->second.mSize, pesIt->second.mPts, pesIt->second.mDts);
          mLastLoadedAudioPts = pesIt->first;
          ++pesIt;

          while (mAudioRender->throttle (getAudioPlayer()->getPts()))
            this_thread::sleep_for (1ms);
          }

        cLog::log (LOGERROR, "exit");
        }).detach();
      }
    //}}}
    //{{{
    void videoLoaderThread() {
    // launch videoLoader thread

      thread ([=]() {
        cLog::setThreadName ("vidL");

        while ((mGopMap.begin() == mGopMap.end()) || !mAudioRender || ! getAudioPlayer()) {
          // wait for analyse to see some videoPes
          if (kVideoLoadDebug)
            cLog::log (LOGINFO, fmt::format ("videoLoader start wait"));
          this_thread::sleep_for (100ms);
          }

        mVideoRender = new cVideoRender (false, 50, "vid", mService->getVideoStreamTypeId(), mService->getVideoPid());

        auto gopIt = mGopMap.begin();
        while (gopIt != mGopMap.end()) {
          if (kVideoLoadDebug)
            cLog::log (LOGINFO, fmt::format ("V gop {}", getFullPtsString (gopIt->first)));

          auto& pesVector = gopIt->second.mPesVector;
          auto pesIt = pesVector.begin();
          while (pesIt != pesVector.end()) {
            if (kVideoLoadDebug)
              cLog::log (LOGINFO, fmt::format ("- V pes {}", getFullPtsString (pesIt->mPts)));
            mVideoRender->decodePes (pesIt->mData, pesIt->mSize, pesIt->mPts, pesIt->mDts);
            ++pesIt;

            while (mVideoRender->throttle (getAudioPlayer()->getPts()))
              this_thread::sleep_for (1ms);
            }

          ++gopIt;
          }

        cLog::log (LOGERROR, "exit");
        }).detach();
      }
    //}}}
    //{{{
    void subtitleLoaderThread() {
    // launch subtitleLoader thread

      thread ([=]() {
        cLog::setThreadName ("subL");

        while ((mSubtitlePesMap.begin() == mSubtitlePesMap.end()) || !mAudioRender || !getAudioPlayer()) {
          // wait for analyse to see some subtitlePes
          if (kSubtitleLoadDebug)
            cLog::log (LOGINFO, fmt::format ("subtitleLoader start wait"));
          this_thread::sleep_for (100ms);
          }

        mSubtitleRender = new cSubtitleRender (false, 0, "sub", mService->getSubtitleStreamTypeId(), mService->getSubtitlePid());

        cLog::log (LOGERROR, "exit");
        }).detach();
      }
    //}}}

    string mFileName;
    cPlayerOptions* mOptions;

    size_t mFileSize = 0;
    cTransportStream::cService* mService = nullptr;

    // pes
    shared_mutex mAudioMutex;
    map <int64_t,sPes> mAudioPesMap;
    int64_t mLastLoadedAudioPts = -1;

    int64_t mNumVideoPes = 0;
    map <int64_t,cGop> mGopMap;
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
      mFilePlayers.emplace_back (filePlayer);
      filePlayer->start();
      }
    //}}}

    cPlayerOptions* getOptions() { return mOptions; }
    vector<cFilePlayer*>& getFilePlayers() { return mFilePlayers; }
    cFilePlayer* getFirstFilePlayer() { return mFilePlayers.front(); }

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
      //{{{  draw motionVectors button
      ImGui::SameLine();
      if (toggleButton ("motion", playerApp.getOptions()->mShowMotionVectors))
        playerApp.toggleShowMotionVectors();
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
        mTL = {(layoutPos.x * viewportWidth) - mSize.x*0.5f, (layoutPos.y * viewportHeight) - mSize.y*0.5f};
        mBR = {(layoutPos.x * viewportWidth) + mSize.x*0.5f, (layoutPos.y * viewportHeight) + mSize.y*0.5f};

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

        cAudioRender* audioRender = playerApp.getFirstFilePlayer()->getAudioRender();
        if (audioRender) {
          cAudioPlayer* audioPlayer = playerApp.getFirstFilePlayer()->getAudioPlayer();
          int64_t playPts = audioRender->getPts();
          if (audioRender->getAudioPlayer()) {
            playPts = audioPlayer->getPts();
            audioPlayer->setMute (false);
            }

          cVideoRender* videoRender = playerApp.getFirstFilePlayer()->getVideoRender();
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
            string ptsFromStartString = getPtsString (audioRender->getPts());

            pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
            ImGui::SetCursorPos (pos);
            ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsFromStartString.c_str());
            ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
            ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsFromStartString.c_str());
            }
            //}}}
          ImGui::PopFont();

          mAudioMeterView.draw (audioRender, playPts,
            ImVec2(mBR.x - ImGui::GetTextLineHeight()*0.5f, mBR.y - ImGui::GetTextLineHeight()*0.25f));
          }

        // invisbleButton over view sub area
        ImGui::SetCursorPos ({0.f,0.f});
        ImVec2 viewSubSize = mSize -
          ImVec2(0.f, ImGui::GetTextLineHeight() * ((layoutPos.y + (layoutScale/2.f) >= 0.99f) ? 3.f : 1.5f));
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
        cFilePlayer* filePlayer = playerApp.getFirstFilePlayer();
        if (filePlayer->getService()) {
          auto it = mViewMap.find (filePlayer->getService()->getSid());
          if (it == mViewMap.end()) {
            mViewMap.emplace (filePlayer->getService()->getSid(), cView (*filePlayer->getService()));
            cLog::log (LOGINFO, fmt::format ("cMultiView::draw add view {}", filePlayer->getService()->getSid()));
            }
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
    void hitControlLeft (cPlayerApp& playerApp) {
    // frame
      playerApp.skipPlay (-90000 / 25);
      }
    //}}}
    //{{{
    void hitShiftLeft (cPlayerApp& playerApp) {
    // 10 frames
      playerApp.skipPlay (-(90000 * 10) / 25);
      }
    //}}}
    //{{{
    void hitLeft (cPlayerApp& playerApp) {
    // second
      playerApp.skipPlay (-90000);
      }
    //}}}

    //{{{
    void hitControlRight (cPlayerApp& playerApp) {
    // frame
      playerApp.skipPlay (90000 / 25);
      }
    //}}}
    //{{{
    void hitShiftRight (cPlayerApp& playerApp) {
    // 10 frames
      playerApp.skipPlay ((90000 * 10) / 25);
      }
    //}}}
    //{{{
    void hitRight (cPlayerApp& playerApp) {
    // second
      playerApp.skipPlay (90000);
      }
    //}}}

    //{{{
    void hitUp (cPlayerApp& playerApp) {
    // 10 seconds
      playerApp.skipPlay (-90000 * 10);
      }
    //}}}
    //{{{
    void hitControlUp (cPlayerApp& playerApp) {
    // minute
      playerApp.skipPlay (-90000 * 60);
      }
    //}}}
    //{{{
    void hitShiftUp (cPlayerApp& playerApp) {
    // 5 minutes
      playerApp.skipPlay (-90000 * 60 * 5);
      }
    //}}}

    //{{{
    void hitDown (cPlayerApp& playerApp) {
    // 10 seconds
      playerApp.skipPlay (90000 * 10);
      }
    //}}}
    //{{{
    void hitControlDown (cPlayerApp& playerApp) {
    // minute
      playerApp.skipPlay (90000 * 60);
      }
    //}}}
    //{{{
    void hitShiftDown (cPlayerApp& playerApp) {
    // 5 minutes
      playerApp.skipPlay (90000 * 60 * 5);
      }
    //}}}

    //{{{
    void hitSpace (cPlayerApp& playerApp) {
      playerApp.togglePlay();
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
        { false, false,  false, ImGuiKey_LeftArrow,  [this,&playerApp]{ hitLeft (playerApp); }},
        { false, false,  true,  ImGuiKey_LeftArrow,  [this,&playerApp]{ hitShiftLeft (playerApp); }},
        { false, true,   false, ImGuiKey_LeftArrow,  [this,&playerApp]{ hitControlLeft (playerApp); }},

        { false, false,  false, ImGuiKey_RightArrow, [this,&playerApp]{ hitRight (playerApp); }},
        { false, true,   false, ImGuiKey_RightArrow, [this,&playerApp]{ hitControlRight (playerApp); }},
        { false, false,  true,  ImGuiKey_RightArrow, [this,&playerApp]{ hitShiftRight (playerApp); }},

        { false, false,  false, ImGuiKey_UpArrow,    [this,&playerApp]{ hitUp (playerApp); }},
        { false, false,  true,  ImGuiKey_UpArrow,    [this,&playerApp]{ hitShiftUp (playerApp); }},
        { false, true,   false, ImGuiKey_UpArrow,    [this,&playerApp]{ hitControlUp (playerApp); }},

        { false, false,  false, ImGuiKey_DownArrow,  [this,&playerApp]{ hitDown (playerApp); }},
        { false, false,  true,  ImGuiKey_DownArrow,  [this,&playerApp]{ hitShiftDown (playerApp); }},
        { false, true,   false, ImGuiKey_DownArrow,  [this,&playerApp]{ hitControlDown (playerApp); }},

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
  }

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

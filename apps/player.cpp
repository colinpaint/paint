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
constexpr bool kPlayVideo = true;
constexpr bool kAudioLoadDebug = false;
constexpr bool kVideoLoadDebug = false;
constexpr bool kSubtitleLoadDebug = false;
constexpr bool kAnalDebug = false;
constexpr bool kAnalTotals = true;

namespace {
  //{{{
  class cFilePlayer {
  public:
    cFilePlayer (string fileName) : mFileName(cFileUtils::resolve (fileName)) {}
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
    int64_t getAudioPlayerPts() { return mAudioRender->getAudioPlayer()->getPts(); }
    //{{{
    void togglePlay() {
      if (getAudioPlayer())
        getAudioPlayer()->togglePlay();
      }
    //}}}
    //{{{
    void skip (int64_t skipPts) {
      if (getAudioPlayer()) {
        int64_t pts = getAudioPlayer()->getPts() + skipPts;
        cLog::log (LOGINFO, fmt::format ("-- skip:{}", getFullPtsString (pts)));
        getAudioPlayer()->skipToPts (pts);
        }
      }
    //}}}

    //{{{
    void start() {
      analyseThread();
      audioLoaderThread();
      if (kPlayVideo)
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

      void load (cVideoRender* videoRender, int64_t loadPts, int64_t gopPts, const string& title) {
        cLog::log (LOGINFO, fmt::format ("{} {} {} gopFirst:{}",
                                         title,
                                         mPesVector.size(),
                                         getPtsString (loadPts),
                                         getPtsString (gopPts)
                                         ));

        for (auto& pes : mPesVector) {
          //cLog::log (LOGINFO, fmt::format ("- pes:{} {}", i, getPtsString (pes.mPts)));
          videoRender->decodePes (pes.mData, pes.mSize, pes.mPts, pes.mDts);
          }
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

        if (kAnalTotals) {
          //{{{  report totals
          cLog::log (LOGINFO, fmt::format ("{}m took {}ms",
            mFileSize/1000000,
            chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count()));

          size_t numVidPes = 0;
          size_t longestGop = 0;
          for (auto& gop : mGopMap) {
            longestGop = max (longestGop, gop.second.mPesVector.size());
            numVidPes += gop.second.mPesVector.size();
            }

          cLog::log (LOGINFO, fmt::format ("- vid:{:6d} {} to {} gop:{} longest:{}",
                                           numVidPes,
                                           getFullPtsString (mGopMap.begin()->second.mPesVector.front().mPts),
                                           getFullPtsString (mGopMap.rbegin()->second.mPesVector.front().mPts),
                                           mGopMap.size(), longestGop));

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
    void audioLoaderThread() {
    // launch audioLoader thread

      thread ([=]() {
        cLog::setThreadName ("audL");

        //{{{  wait for first audioPes
        this_thread::sleep_for (1ms);
        while (mAudioPesMap.begin() == mAudioPesMap.end()) {
          cLog::log (LOGINFO, fmt::format ("audioLoader wait analyse"));
          this_thread::sleep_for (1ms);
          }
        //}}}
        mAudioRender = new cAudioRender (false, 59, 4, true,
                                         mService->getAudioStreamTypeId(), mService->getAudioPid());

        // load first audioPes, creates audioPlayer
        sPes pes = mAudioPesMap.begin()->second;
        cLog::log (LOGINFO, fmt::format ("first load:{}", getFullPtsString (pes.mPts)));
        mAudioRender->decodePes (pes.mData, pes.mSize, pes.mPts, pes.mDts);
        //{{{  audioPlayer ok?
        if (!getAudioPlayer())
          cLog::log (LOGERROR, fmt::format ("audioLoader wait player"));
        //}}}

        while (true) {
          int64_t loadPts = mAudioRender->load (getAudioPlayerPts());
          if (loadPts == -1) {
            // all loaded
            this_thread::sleep_for (1ms);
            continue;
            }

          { // lock
          unique_lock<shared_mutex> lock (mAudioMutex);
          auto it = mAudioPesMap.upper_bound (loadPts);
          if (it == mAudioPesMap.begin()) {
            this_thread::sleep_for (1ms);
            continue;
            }
          --it;
          pes = it->second;
          }
          if (kAudioLoadDebug)
            cLog::log (LOGINFO, fmt::format ("- loader chunk:{}:{}",
                                             getFullPtsString (pes.mPts), getFullPtsString (loadPts)));
          mAudioRender->decodePes (pes.mData, pes.mSize, pes.mPts, pes.mDts);
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

        //{{{  wait for first video pes
        while ((mGopMap.begin() == mGopMap.end()) || !mAudioRender || ! getAudioPlayer()) {
          if (kVideoLoadDebug)
            cLog::log (LOGINFO, fmt::format ("videoLoader start wait"));
          this_thread::sleep_for (100ms);
          }
        //}}}
        mVideoRender = new cVideoRender (false, 100, 24,
                                         mService->getVideoStreamTypeId(), mService->getVideoPid());
        while (true) {
          int64_t loadPts = getAudioPlayerPts();
          auto nextGopIt = mGopMap.upper_bound (loadPts);

          auto gopIt = nextGopIt;
          if (gopIt != mGopMap.begin())
            --gopIt;

          if (!mVideoRender->getFrameAtPts (loadPts)) {
            gopIt->second.load (mVideoRender, loadPts, gopIt->first, "this");
            continue;
            }

          if ((nextGopIt->first - loadPts) <= (3 * 3600)) {
            if (!mVideoRender->getFrameAtPts (nextGopIt->first)) {
              nextGopIt->second.load (mVideoRender, loadPts, nextGopIt->first, "next");
              continue;
              }
            }

          this_thread::sleep_for (1ms);
          }

        cLog::log (LOGERROR, "exit");
        }).detach();
      }
    //}}}
    //{{{
    //void videoLoaderThread() {
    //// launch videoLoader thread

      //thread ([=]() {
        //cLog::setThreadName ("vidL");

        //while ((mGopMap.begin() == mGopMap.end()) || !mAudioRender || ! getAudioPlayer()) {
          //// wait for analyse to see some videoPes
          //if (kVideoLoadDebug)
            //cLog::log (LOGINFO, fmt::format ("videoLoader start wait"));
          //this_thread::sleep_for (100ms);
          //}

        //mVideoRender = new cVideoRender (false, 50, 56,
                                         //mService->getVideoStreamTypeId(), mService->getVideoPid());

        //auto gopIt = mGopMap.begin();
        //while (gopIt != mGopMap.end()) {
          //if (kVideoLoadDebug)
            //cLog::log (LOGINFO, fmt::format ("V gop {}", getFullPtsString (gopIt->first)));

          //auto& pesVector = gopIt->second.mPesVector;
          //auto pesIt = pesVector.begin();
          //while (pesIt != pesVector.end()) {
            //if (kVideoLoadDebug)
              //cLog::log (LOGINFO, fmt::format ("- V pes {}", getFullPtsString (pesIt->mPts)));
            //mVideoRender->decodePes (pesIt->mData, pesIt->mSize, pesIt->mPts, pesIt->mDts);
            //++pesIt;

            //while (mVideoRender->throttle (getAudioPlayer()->getPts()))
              //this_thread::sleep_for (1ms);
            //}

          //++gopIt;
          //}

        //cLog::log (LOGERROR, "exit");
        //}).detach();
      //}
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

        mSubtitleRender = new cSubtitleRender (
          false, 0, 0, mService->getSubtitleStreamTypeId(), mService->getSubtitlePid());

        cLog::log (LOGERROR, "exit");
        }).detach();
      }
    //}}}

    string mFileName;

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
  class cPlayerOptions : public cApp::cOptions {
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

      cFilePlayer* filePlayer = new cFilePlayer (fileName);
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
      //cLog::log (LOGINFO, fmt::format ("skip:{}", skipPts));
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
    class cRenderView {
    public:
      void draw (cAudioRender* audioRender, cVideoRender* videoRender, int64_t playerPts, ImVec2 pos) {

        float ptsScale = 6.f / (videoRender ? videoRender->getPtsDuration() : 90000/25);
        if (audioRender) {
          // lock audio during iterate
          shared_lock<shared_mutex> lock (audioRender->getSharedMutex());
          for (auto& frame : audioRender->getFramesMap()) {
            //{{{  draw audio frames
            float posL = pos.x + (frame.second->getPts() - playerPts) * ptsScale;
            float posR = pos.x + ((frame.second->getPts() - playerPts + frame.second->getPtsDuration()) * ptsScale) - 1.f;

            cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame.second);
            ImGui::GetWindowDrawList()->AddRectFilled (
              {posL, pos.y - addValue (audioFrame->getSimplePower(), mMaxPower, 2.f * ImGui::GetTextLineHeight())},
              {posR, pos.y },
              0x8000ff00);
            }
            //}}}
          }

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
              {posL, pos.y - addValue ((float)videoFrame->getPesSize(), mMaxPesSize, 2.f * ImGui::GetTextLineHeight())},
              {posR, pos.y },
              (videoFrame->mFrameType == 'I') ?                  // I - white
                0xffffffff : (videoFrame->mFrameType == 'P') ?   // P - purple
                  0xffFF40ff : (videoFrame->mFrameType == 'B') ? // B - blue
                    0xffFF4040 : 0xff40FFFF);                    // ? = yellow
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
          // get audio player pts
          cAudioPlayer* audioPlayer = playerApp.getFirstFilePlayer()->getAudioPlayer();
          if (audioPlayer) {
            int64_t playPts = audioRender->getPts();
            if (audioRender->getAudioPlayer()) {
              playPts = audioPlayer->getPts();
              audioPlayer->setMute (false);
              }

            // get videoFrame
            cVideoRender* videoRender = nullptr;
            if (kPlayVideo) {
              videoRender = playerApp.getFirstFilePlayer()->getVideoRender();
              if (videoRender) {
                cVideoFrame* videoFrame = videoRender->getVideoFrameAtOrAfterPts (playPts);
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
                  cTexture& texture = videoFrame->getTexture (playerApp.getGraphics());
                  texture.setSource();

                  // ensure quad is created
                  if (!mVideoQuad)
                    mVideoQuad = playerApp.getGraphics().createQuad (videoFrame->getSize());

                  // draw quad
                  mVideoQuad->draw();

                  //}}}
                  if (playerApp.getOptions()->mShowSubtitle) {
                    //{{{  draw subtitles
                    cSubtitleRender& subtitleRender =
                      dynamic_cast<cSubtitleRender&> (mService.getStream (cRenderStream::eSubtitle).getRender());

                    subtitleShader->use();
                    for (size_t line = 0; line < subtitleRender.getNumLines(); line++) {
                      cSubtitleImage& subtitleImage = subtitleRender.getImage (line);
                      if (!mSubtitleTextures[line])
                        mSubtitleTextures[line] = playerApp.getGraphics().createTexture (cTexture::eRgba, subtitleImage.getSize());
                      mSubtitleTextures[line]->setSource();

                      // update subtitle texture if image dirty
                      if (subtitleImage.isDirty())
                        mSubtitleTextures[line]->setPixels (subtitleImage.getPixels(), nullptr);
                      subtitleImage.setDirty (false);

                      float xpos = (float)subtitleImage.getXpos() / videoFrame->getWidth();
                      float ypos = (float)(videoFrame->getHeight() - subtitleImage.getYpos()) / videoFrame->getHeight();
                      model.setTranslate ({(layoutPos.x + ((xpos - 0.5f) * layoutScale)) * viewportWidth,
                                           ((1.0f - layoutPos.y) + ((ypos - 0.5f) * layoutScale)) * viewportHeight});
                      subtitleShader->setModelProjection (model, projection);

                      // ensure quad is created (assumes same size) and draw
                      if (!mSubtitleQuads[line])
                        mSubtitleQuads[line] = playerApp.getGraphics().createQuad (mSubtitleTextures[line]->getSize());
                      mSubtitleQuads[line]->draw();
                      }
                    }
                    //}}}
                  if (playerApp.getOptions()->mShowMotionVectors && (mSelect == eSelectedFull)) {
                    //{{{  draw motion vectors
                    size_t numMotionVectors;
                    AVMotionVector* mv = (AVMotionVector*)(videoFrame->getMotionVectors (numMotionVectors));
                    if (numMotionVectors) {
                      for (size_t i = 0; i < numMotionVectors; i++) {
                        ImGui::GetWindowDrawList()->AddLine (
                          mTL + ImVec2(mv->src_x * viewportWidth / videoFrame->getWidth(),
                                       mv->src_y * viewportHeight / videoFrame->getHeight()),
                          mTL + ImVec2((mv->src_x + (mv->motion_x / mv->motion_scale)) * viewportWidth / videoFrame->getWidth(),
                                        (mv->src_y + (mv->motion_y / mv->motion_scale)) * viewportHeight / videoFrame->getHeight()),
                          mv->source > 0 ? 0xc0c0c0c0 : 0xc000c0c0, 1.f);
                        mv++;
                        }
                      }
                    }
                    //}}}
                  }
                }
              }

            //{{{  draw fileName and playPts with largeFont
            ImGui::PushFont (playerApp.getLargeFont());

            string title = playerApp.getFirstFilePlayer()->getFileName();
            ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
            ImGui::SetCursorPos (pos);
            ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
            ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
            ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());

            pos.y += ImGui::GetTextLineHeight() * 1.5f;

            string ptsString = getPtsString (playPts);
            pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
            ImGui::SetCursorPos (pos);
            ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsString.c_str());
            ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
            ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsString.c_str());

            ImGui::PopFont();
            //}}}
            //{{{  draw audio meter
            mAudioMeterView.draw (audioRender, playPts,
              ImVec2(mBR.x - ImGui::GetTextLineHeight()*0.5f, mBR.y - ImGui::GetTextLineHeight()*0.25f));
            //}}}
            //{{{  draw render frames graphic
            mRenderView.draw (audioRender, videoRender, playPts,
                              ImVec2((mTL.x + mBR.x)/2.f, mBR.y - ImGui::GetTextLineHeight()*0.25f));
            //}}}
            }
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
      cAudioMeterView mAudioMeterView;
      cRenderView mRenderView;

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
        { false, false,  false, ImGuiKey_LeftArrow,  [this,&playerApp]{ playerApp.skipPlay (-90000/25); }},
        { false, false,  true,  ImGuiKey_LeftArrow,  [this,&playerApp]{ playerApp.skipPlay (-90000/5); }},
        { false, true,   false, ImGuiKey_LeftArrow,  [this,&playerApp]{ playerApp.skipPlay (-90000*10/25); }},

        { false, false,  false, ImGuiKey_RightArrow, [this,&playerApp]{ playerApp.skipPlay (90000/25); }},
        { false, true,   false, ImGuiKey_RightArrow, [this,&playerApp]{ playerApp.skipPlay (90000/5); }},
        { false, false,  true,  ImGuiKey_RightArrow, [this,&playerApp]{ playerApp.skipPlay (90000*10/25); }},

        { false, false,  false, ImGuiKey_UpArrow,    [this,&playerApp]{ playerApp.skipPlay (-90000); }},
        { false, false,  true,  ImGuiKey_UpArrow,    [this,&playerApp]{ playerApp.skipPlay (-90000*10); }},
        { false, true,   false, ImGuiKey_UpArrow,    [this,&playerApp]{ playerApp.skipPlay (-90000*60); }},

        { false, false,  false, ImGuiKey_DownArrow,  [this,&playerApp]{ playerApp.skipPlay (90000); }},
        { false, false,  true,  ImGuiKey_DownArrow,  [this,&playerApp]{ playerApp.skipPlay (90000*10); }},
        { false, true,   false, ImGuiKey_DownArrow,  [this,&playerApp]{ playerApp.skipPlay (90000*60); }},

        { false, false,  false, ImGuiKey_Space,      [this,&playerApp]{ playerApp.togglePlay(); }},
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

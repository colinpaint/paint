// telly.cpp - telly app
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

// utils
#include "../date/include/date/date.h"
#include "../common/basicTypes.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"
#include "../common/iOptions.h"

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
#include "../dvb/cDvbMultiplex.h"
#include "../dvb/cDvbSource.h"
#include "../dvb/cTransportStream.h"
#include "../dvb/cVideoRender.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cPlayer.h"

using namespace std;
//}}}

namespace {
  //{{{  const string kRootDir
    #ifdef _WIN32
      const string kRootDir = "/tv/";
    #else
      const string kRootDir = "/home/pi/tv/";
    #endif
    //}}}
  //{{{
  const vector <cDvbMultiplex> kDvbMultiplexes = {
      {"hd", 626000000,
        {"BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "Channel 5 HD", "ITV1 HD", "Channel 4 HD"},
        {"bbc1",          "bbc2",       "bbc3",         "bbc4",        "c5",           "itv",     "c4"}
      },

      {"itv", 650000000,
        {"ITV1",   "ITV2",   "ITV3",   "ITV4",   "Channel 4", "More 4",  "Film4" ,  "E4",   "Channel 5"},
        {"itv1sd", "itv2sd", "itv3sd", "itv4sd", "c4sd",      "more4sd", "film4sd", "e4sd", "c5sd"}
      },

      {"bbc", 674000000,
        {"BBC ONE S West", "BBC TWO", "BBC THREE", "BBC FOUR", "BBC NEWS"},
        {"bbc1sd",         "bbc2sd",  "bbc3sd",    "bbc4sd",   "bbcNewsSd"}
      }
    };
  //}}}
  //{{{
  class cTellyOptions : public cApp::cOptions,
                        public cTransportStream::cOptions,
                        public cRender::cOptions,
                        public cAudioRender::cOptions,
                        public cVideoRender::cOptions,
                        public cFFmpegVideoDecoder::cOptions {
  public:
    virtual ~cTellyOptions() = default;

    string getString() const { return "none epg sub motion hd|bbc|itv|*.ts"; }

    // vars
    bool mShowSubtitle = false;
    bool mShowMotionVectors = false;

    bool mShowAllServices = true;
    bool mShowEpg = false;

    cDvbMultiplex mMultiplex = kDvbMultiplexes[0];
    string mFileName;
    };
  //}}}
  //{{{
  class cTellyApp : public cApp {
  public:
    cTellyApp (cTellyOptions* options, iUI* ui) : cApp ("telly", options, ui), mOptions(options) {}
    virtual ~cTellyApp() = default;

    cTellyOptions* getOptions() { return mOptions; }
    bool hasTransportStream() { return mTransportStream != nullptr; }
    cTransportStream& getTransportStream() { return *mTransportStream; }

    // liveDvbSource
    bool hasDvbSource() const { return mDvbSource; }
    cDvbSource& getDvbSource() { return *mDvbSource; }
    //{{{
    void liveDvbSource (const cDvbMultiplex& multiplex, cTellyOptions* options) {

      cLog::log (LOGINFO, fmt::format ("multiplex:{} {:4.1f} record:{}",
                                       multiplex.mName, multiplex.mFrequency/1000.f, options->mRecordRoot));

      mFileSource = false;
      mMultiplex = multiplex;
      mRecordRoot = options->mRecordRoot;
      if (multiplex.mFrequency)
        mDvbSource = new cDvbSource (multiplex.mFrequency, 0);

      mTransportStream = new cTransportStream (multiplex, options,
        [&](cTransportStream::cService& service) noexcept {
          cLog::log (LOGINFO, fmt::format ("addService {}", service.getSid()));
          if (options->mShowAllServices)
            if (service.getStream (cStream::eVideo).isDefined()) {
              service.enableStream (cStream::eVideo);
              service.enableStream (cStream::eAudio);
              service.enableStream (cStream::eSubtitle);
              }
          },
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo, bool skip) noexcept {
          cStream* stream = service.getStreamByPid (pidInfo.getPid());
          if (stream && stream->isEnabled())
            if (stream->getRender().decodePes (pidInfo.mBuffer, pidInfo.getBufSize(),
                                               pidInfo.getPts(), pidInfo.getDts(),
                                               pidInfo.mStreamPos, skip))
              // transferred ownership of mBuffer to render, create new one
              pidInfo.mBuffer = (uint8_t*)malloc (pidInfo.mBufSize);
          //cLog::log (LOGINFO, fmt::format ("pes sid:{} pid:{} size:{}",
          //                                 service.getSid(), pidInfo.getPid(), pidInfo.getBufSize()));
          });

      if (!mTransportStream) {
        //{{{  error, return
        cLog::log (LOGERROR, "cTellyApp::setLiveDvbSource - failed to create liveDvbSource");
        return;
        }
        //}}}

      FILE* mFile = options->mRecordAllServices ?
        fopen ((mRecordRoot + mMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

      #ifdef _WIN32
        //{{{  windows
        mDvbSource->run();

        int64_t streamPos = 0;
        int blockSize = 0;

        while (true) {
          auto ptr = mDvbSource->getBlockBDA (blockSize);
          if (blockSize) {
            //  read and demux block
            streamPos += mTransportStream->demux (ptr, blockSize, streamPos, false);
            if (options->mRecordAllServices && mFile)
              fwrite (ptr, 1, blockSize, mFile);
            mDvbSource->releaseBlock (blockSize);
            }
          else
            this_thread::sleep_for (1ms);
          }
        //}}}
      #else
        //{{{  linux
        constexpr int kDvrReadChunkSize = 188 * 64;
        uint8_t* chunk = new uint8_t[kDvrReadChunkSize];

        uint64_t streamPos = 0;
        while (true) {
          int bytesRead = mDvbSource->getBlock (chunk, kDvrReadChunkSize);
          if (bytesRead) {
            streamPos += mTransportStream->demux (chunk, bytesRead, 0, false);
            if (options->mRecordAllServices && mFile)
              fwrite (chunk, 1, bytesRead, mFile);
            }
          else
            cLog::log (LOGINFO, fmt::format ("liveDvbSource - no bytes"));
          }

        delete[] chunk;
        //}}}
      #endif

      if (mFile)
        fclose (mFile);
      }
    //}}}
    //{{{
    void liveDvbSourceThread (const cDvbMultiplex& multiplex, cTellyOptions* options) {

      cLog::log (LOGINFO, fmt::format ("thread - multiplex:{} {:4.1f} record:{}",
                                       multiplex.mName, multiplex.mFrequency/1000.f, options->mRecordRoot));

      mFileSource = false;
      mMultiplex = multiplex;
      mRecordRoot = options->mRecordRoot;
      if (multiplex.mFrequency)
        mDvbSource = new cDvbSource (multiplex.mFrequency, 0);

      mTransportStream = new cTransportStream (multiplex, options,
        [&](cTransportStream::cService& service) noexcept {
          cLog::log (LOGINFO, fmt::format ("addService {}", service.getSid()));
          if (options->mShowAllServices)
            if (service.getStream (cStream::eVideo).isDefined()) {
              service.enableStream (cStream::eVideo);
              service.enableStream (cStream::eAudio);
              service.enableStream (cStream::eSubtitle);
              }
          },
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo, bool skip) noexcept {
          cStream* stream = service.getStreamByPid (pidInfo.getPid());
          if (stream && stream->isEnabled())
            if (stream->getRender().decodePes (pidInfo.mBuffer, pidInfo.getBufSize(),
                                               pidInfo.getPts(), pidInfo.getDts(),
                                               pidInfo.mStreamPos, skip))
              // transferred ownership of mBuffer to render, create new one
              pidInfo.mBuffer = (uint8_t*)malloc (pidInfo.mBufSize);

          //cLog::log (LOGINFO, fmt::format ("pes sid:{} pid:{} size:{}",
          //                                 service.getSid(), pidInfo.getPid(), pidInfo.getBufSize()));
          });

      if (!mTransportStream) {
        //{{{  error, return
        cLog::log (LOGERROR, "cTellyApp::setLiveDvbSource - failed to create liveDvbSource");
        return;
        }
        //}}}

      FILE* mFile = options->mRecordAllServices ?
        fopen ((mRecordRoot + mMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

      mLiveThread = thread ([=]() {
        cLog::setThreadName ("dvb ");

        #ifdef _WIN32
          //{{{  windows
          SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
          mDvbSource->run();

          int64_t streamPos = 0;
          int blockSize = 0;

          while (true) {
            auto ptr = mDvbSource->getBlockBDA (blockSize);
            if (blockSize) {
              //  read and demux block
              streamPos += mTransportStream->demux (ptr, blockSize, streamPos, false);
              if (options->mRecordAllServices && mFile)
                fwrite (ptr, 1, blockSize, mFile);
              mDvbSource->releaseBlock (blockSize);
              }
            else
              this_thread::sleep_for (1ms);
            }
          //}}}
        #else
          //{{{  linux
          sched_param sch_params;
          sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
          pthread_setschedparam (mLiveThread.native_handle(), SCHED_RR, &sch_params);

          constexpr int kDvrReadChunkSize = 188 * 64;
          uint8_t* chunk = new uint8_t[kDvrReadChunkSize];

          uint64_t streamPos = 0;
          while (true) {
            int bytesRead = mDvbSource->getBlock (chunk, kDvrReadChunkSize);
            if (bytesRead) {
              streamPos += mTransportStream->demux (chunk, bytesRead, 0, false);
              if (options->mRecordAllServices && mFile)
                fwrite (chunk, 1, bytesRead, mFile);
              }
            else
              cLog::log (LOGINFO, fmt::format ("liveDvbSource - no bytes"));
            }

          delete[] chunk;
          //}}}
        #endif

        if (mFile)
          fclose (mFile);

        cLog::log (LOGINFO, "exit");
        });

      mLiveThread.detach();
      }
    //}}}

    // fileSource
    //{{{
    void fileSource (const string& filename, cTellyOptions* options) {

      // create transportStream
      mTransportStream = new cTransportStream ({"file", 0, {}, {}}, options,
        [&](cTransportStream::cService& service) noexcept {
          cLog::log (LOGINFO, fmt::format ("addService {}", service.getSid()));
          if (options->mShowAllServices)
            if (service.getStream (cStream::eVideo).isDefined()) {
              service.enableStream (cStream::eVideo);
              service.enableStream (cStream::eAudio);
              service.enableStream (cStream::eSubtitle);
              }
          },
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo, bool skip) noexcept {
          cStream* stream = service.getStreamByPid (pidInfo.getPid());
          if (stream && stream->isEnabled())
            if (stream->getRender().decodePes (pidInfo.mBuffer, pidInfo.getBufSize(),
                                               pidInfo.getPts(), pidInfo.getDts(),
                                               pidInfo.mStreamPos, skip))
              // transferred ownership of mBuffer to render, create new one
              pidInfo.mBuffer = (uint8_t*)malloc (pidInfo.mBufSize);
          //cLog::log (LOGINFO, fmt::format ("pes sid:{} pid:{} size:{}",
          //                                 service.getSid(), pidInfo.getPid(), pidInfo.getBufSize()));
          });
      if (!mTransportStream) {
        //{{{  error, return
        cLog::log (LOGERROR, "fileSource cTransportStream create failed");
        return;
        }
        //}}}

      // open fileSource
      mOptions->mFileName = cFileUtils::resolve (filename);
      FILE* file = fopen (mOptions->mFileName.c_str(), "rb");
      if (!file) {
        //{{{  error, return
        cLog::log (LOGERROR, fmt::format ("fileSource failed to open {}", mOptions->mFileName));
        return;
        }
        //}}}

      mFileSource = true;

      // create fileRead thread
      thread ([=]() {
        cLog::setThreadName ("file");

        int64_t mStreamPos = 0;
        int64_t mFilePos = 0;
        size_t chunkSize = 188 * 256;
        uint8_t* chunk = new uint8_t[chunkSize];
        while (true) {
          while (mTransportStream->throttle())
            this_thread::sleep_for (1ms);

          // seek and read chunk from file
          bool skip = mStreamPos != mFilePos;
          if (skip) {
            //{{{  seek to mStreamPos
            if (fseek (file, (long)mStreamPos, SEEK_SET))
              cLog::log (LOGERROR, fmt::format ("seek failed {}", mStreamPos));
            else {
              cLog::log (LOGINFO, fmt::format ("seek {}", mStreamPos));
              mFilePos = mStreamPos;
              }
            }
            //}}}
          size_t bytesRead = fread (chunk, 1, chunkSize, file);
          mFilePos = mFilePos + bytesRead;
          if (bytesRead > 0)
            mStreamPos += mTransportStream->demux (chunk, bytesRead, mStreamPos, skip);
          else
            break;
          }

        fclose (file);
        delete[] chunk;

        cLog::log (LOGERROR, "exit");
        }).detach();
      }
    //}}}

    // actions
    void setShowEpg (bool showEpg) { mOptions->mShowEpg = showEpg; }
    void toggleShowSubtitle() { mOptions->mShowSubtitle = !mOptions->mShowSubtitle; }
    void toggleShowMotionVectors() { mOptions->mShowMotionVectors = !mOptions->mShowMotionVectors; }

    void drop (const vector<string>& dropItems) { (void)dropItems; }

  private:
    cTellyOptions* mOptions;
    cTransportStream* mTransportStream = nullptr;

    // liveDvbSource
    thread mLiveThread;
    cDvbSource* mDvbSource = nullptr;
    cDvbMultiplex mMultiplex;
    string mRecordRoot;

    FILE* mFile = nullptr;
    bool mFileSource = false;
    };
  //}}}
  //{{{
  class cTellyUI : public cApp::iUI {
  public:
    cTellyUI() : mMultiView (*(new cMultiView())) {}
    virtual ~cTellyUI() = default;

    //{{{
    void draw (cApp& app) {

      ImGui::SetKeyboardFocusHere();

      app.getGraphics().clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                                (int32_t)ImGui::GetIO().DisplaySize.y});

      // draw UI
      ImGui::SetNextWindowPos ({0.f,0.f});
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
      ImGui::Begin ("telly", nullptr, ImGuiWindowFlags_NoTitleBar |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                      ImGuiWindowFlags_NoBackground);

      cTellyApp& tellyApp = (cTellyApp&)app;
      if (tellyApp.hasTransportStream()) {
        cTransportStream& transportStream = tellyApp.getTransportStream();

        // draw multiView piccies
        mMultiView.draw (tellyApp, transportStream);

        // draw menu
        ImGui::SetCursorPos ({0.f, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeight() * 1.5f});
        ImGui::BeginChild ("menu", {0.f, ImGui::GetTextLineHeight() * 1.5f},
                           ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoBackground);

        ImGui::SetCursorPos ({0.f,0.f});
        uint8_t index = mTabIndex;
        if (maxOneButton (kTabNames, index, {0.f,0.f}, true))
          hitTab (tellyApp, index);
        //{{{  draw msubtitle button
        ImGui::SameLine();
        if (toggleButton ("sub", tellyApp.getOptions()->mShowSubtitle))
          tellyApp.toggleShowSubtitle();
        //}}}
        if (tellyApp.getOptions()->mHasMotionVectors) {
          //{{{  draw mmotionVectors button
          ImGui::SameLine();
          if (toggleButton ("motion", tellyApp.getOptions()->mShowMotionVectors))
            tellyApp.toggleShowMotionVectors();
          }
          //}}}
        if (tellyApp.getPlatform().hasFullScreen()) {
          //{{{  draw mfullScreen button
          ImGui::SameLine();
          if (toggleButton ("full", tellyApp.getPlatform().getFullScreen()))
            tellyApp.getPlatform().toggleFullScreen();
          }
          //}}}
        if (tellyApp.getPlatform().hasVsync()) {
          //{{{  draw mvsync button
          ImGui::SameLine();
          if (toggleButton ("vsync", tellyApp.getPlatform().getVsync()))
            tellyApp.getPlatform().toggleVsync();
          }
          //}}}
        //{{{  draw mframeRate info
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
        //}}}
        if (tellyApp.hasDvbSource()) {
          //{{{  draw mdvbSource signal,errors
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{} {}", tellyApp.getDvbSource().getTuneString(),
                                                        tellyApp.getDvbSource().getStatusString()).c_str());
          }
          //}}}
        if (transportStream.getNumErrors()) {
          //{{{  draw mtransportStream errors
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("error:{}", tellyApp.getTransportStream().getNumErrors()).c_str());
          }
          //}}}
        ImGui::EndChild();

        //{{{  draw tab subMenu
        switch (mTabIndex) {
          case ePids:
            ImGui::PushFont (tellyApp.getMonoFont());
            drawPids (transportStream);
            ImGui::PopFont();
            break;

          case eEpg:
          default:
            break;
          }
        //}}}
        if (transportStream.hasFirstTdt()) {
          //{{{  draw mclock
          //ImGui::TextUnformatted (transportStream.getTdtString().c_str());
          ImGui::SetCursorPos ({ ImGui::GetWindowWidth() - 90.f, 0.f} );
          clockButton ("clock", transportStream.getNowTdt(), { 80.f, 80.f });
          }
          //}}}
        }
      ImGui::End();

      keyboard (tellyApp);
      }
    //}}}

  private:
    enum eTab { eNone, eEpg, ePids };
    static inline const vector <string> kTabNames = { "none", "epg", "pids" };
    static inline const size_t kMaxSubtitleLines = 4;

    //{{{
    class cFramesView {
    public:
      void draw (cAudioRender& audioRender, cVideoRender& videoRender, int64_t playerPts, ImVec2 pos) {
        float ptsScale = mPixelsPerVideoFrame / videoRender.getPtsDuration();

        { // lock video during iterate
        shared_lock<shared_mutex> lock (videoRender.getSharedMutex());
        for (auto& frame : videoRender.getFramesMap()) {
          //{{{  draw video frames
          float posL = pos.x + (frame.second->getPts() - playerPts) * ptsScale;
          float posR = pos.x + ((frame.second->getPts() - playerPts + frame.second->getPtsDuration()) * ptsScale) - 1.f;

          // pesSize I white / P purple / B blue - ABGR color
          cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame.second);
          ImGui::GetWindowDrawList()->AddRectFilled (
            { posL, pos.y - addValue ((float)videoFrame->getPesSize(), mMaxPesSize, mMaxDisplayPesSize,
                                         kLines * ImGui::GetTextLineHeight()) },
            { posR, pos.y },
            (videoFrame->mFrameType == 'I') ?
              0xffffffff : (videoFrame->mFrameType == 'P') ?
                0xffFF40ff : 0xffFF4040);
          }
          //}}}
        }

        { // lock audio during iterate
        shared_lock<shared_mutex> lock (audioRender.getSharedMutex());
        for (auto& frame : audioRender.getFramesMap()) {
          //{{{  draw audio frames
          float posL = pos.x + (frame.second->getPts() - playerPts) * ptsScale;
          float posR = pos.x + ((frame.second->getPts() - playerPts + frame.second->getPtsDuration()) * ptsScale) - 1.f;

          cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame.second);
          ImGui::GetWindowDrawList()->AddRectFilled (
            { posL, pos.y - addValue (audioFrame->getSimplePower(), mMaxPower, mMaxDisplayPower,
                                      kLines * ImGui::GetTextLineHeight()) },
            { posR, pos.y },
            0x8000ff00);
          }
          //}}}
        }

        // draw playPts centre bar
        ImGui::GetWindowDrawList()->AddRectFilled (
          { pos.x-1.f, pos.y - (kLines * ImGui::GetTextLineHeight()) },
          { pos.x+1.f, pos.y },
          0xffffffff);

        // agc scaling back to max display values from max values
        agc (mMaxPesSize, mMaxDisplayPesSize, 100.f, 10000.f);
        agc (mMaxPower, mMaxDisplayPower, 0.001f, 0.1f);
        }

    private:
      //{{{
      float addValue (float value, float& maxValue, float& maxDisplayValue, float scale) {

        if (value > maxValue)
          maxValue = value;
        if (value > maxDisplayValue)
          maxDisplayValue = value;

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
      const float kLines = 2.f;
      const float mPixelsPerVideoFrame = 6.f;
      const float mPixelsPerAudioChannel = 6.f;

      float mMaxPower = 0.f;
      float mMaxDisplayPower = 0.f;

      float mMaxPesSize = 0.f;
      float mMaxDisplayPesSize = 0.f;
      };
    //}}}
    //{{{
    class cAudioMeterView {
    public:
      void draw (cAudioRender& audioRender, int64_t playerPts, ImVec2 pos) {
        cAudioFrame* audioFrame = audioRender.getAudioFrameAtPts (playerPts);
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
          float x = pos.x - (drawChannels * mPixelsPerAudioChannel);
          pos.y += 1.f;
          float height = 8.f * ImGui::GetTextLineHeight();
          for (size_t i = 0; i < drawChannels; i++) {
            ImGui::GetWindowDrawList()->AddRectFilled (
              {x, pos.y - (audioFrame->mPowerValues[channelOrder[i]] * height)},
              {x + mPixelsPerAudioChannel - 1.f, pos.y},
              0xff00ff00);
            x += mPixelsPerAudioChannel;
            }

          // draw 5.1 woofer red
          if (audio51)
            ImGui::GetWindowDrawList()->AddRectFilled (
              {pos.x - (5 * mPixelsPerAudioChannel), pos.y - (audioFrame->mPowerValues[channelOrder[5]] * height)},
              {pos.x - 1.f, pos.y},
              0x800000ff);
          }
        }

    private:
      //  vars
      const float mAudioLines = 1.f;
      const float mPixelsPerAudioChannel = 6.f;
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
      bool draw (cTellyApp& tellyApp, cTransportStream& transportStream,
                 bool selectFull, size_t viewIndex, size_t numViews,
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

        bool enabled = mService.getStream (cStream::eVideo).isEnabled();
        if (enabled) {
          //{{{  get audio playPts
          int64_t playPts = mService.getStream (cStream::eAudio).getRender().getPts();
          if (mService.getStream (cStream::eAudio).isEnabled()) {
            // get playPts from audioStream
            cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (cStream::eAudio).getRender());
            if (audioRender.getPlayer())
              playPts = audioRender.getPlayer()->getPts();
            }
          //}}}
          if (!selectFull || (mSelect != eUnselected)) {
            cVideoRender& videoRender = dynamic_cast<cVideoRender&>(mService.getStream (cStream::eVideo).getRender());
            cVideoFrame* videoFrame = videoRender.getVideoFrameAtOrAfterPts (playPts);
            if (videoFrame) {
              //{{{  video, subtitle, motionVectors
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
              cTexture& texture = videoFrame->getTexture (tellyApp.getGraphics());
              texture.setSource();

              // ensure quad is created
              if (!mVideoQuad)
                mVideoQuad = tellyApp.getGraphics().createQuad (videoFrame->getSize());

              // draw quad
              mVideoQuad->draw();

              if (tellyApp.getOptions()->mShowSubtitle) {
                //{{{  draw subtitles
                cSubtitleRender& subtitleRender =
                  dynamic_cast<cSubtitleRender&> (mService.getStream (cStream::eSubtitle).getRender());

                subtitleShader->use();
                for (size_t line = 0; line < subtitleRender.getNumLines(); line++) {
                  cSubtitleImage& subtitleImage = subtitleRender.getImage (line);
                  if (!mSubtitleTextures[line])
                    mSubtitleTextures[line] = tellyApp.getGraphics().createTexture (cTexture::eRgba, subtitleImage.getSize());
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
                    mSubtitleQuads[line] = tellyApp.getGraphics().createQuad (mSubtitleTextures[line]->getSize());
                  mSubtitleQuads[line]->draw();
                  }
                }
                //}}}

              if (tellyApp.getOptions()->mShowMotionVectors && (mSelect == eSelectedFull)) {
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
              //}}}
            if (mService.getStream (cStream::eAudio).isEnabled()) {
              //{{{  audio mute, audioMeter, framesGraphic
              cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (cStream::eAudio).getRender());

              // mute audio of unselected
              if (audioRender.getPlayer())
                audioRender.getPlayer()->setMute (mSelect == eUnselected);

              // draw audioMeter graphic
              mAudioMeterView.draw (audioRender, playPts,
                                    ImVec2(mBR.x - ImGui::GetTextLineHeight()*0.5f,
                                           mBR.y - ImGui::GetTextLineHeight()*0.25f));

              // draw frames graphic
              if (mSelect == eSelectedFull)
                mFramesView.draw (audioRender, videoRender, playPts,
                                  ImVec2((mTL.x + mBR.x)/2.f,
                                         mBR.y - ImGui::GetTextLineHeight()*0.25f));
              }
              //}}}
            }
          }

        // largeFont for title and pts
        ImGui::PushFont (tellyApp.getLargeFont());
        string title = mService.getName();
        if (!enabled || (mSelect == eSelectedFull))
          title += " " + mService.getNowTitleString();
        //{{{  draw dropShadow title topLeft
        ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
        ImGui::SetCursorPos (pos);
        ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
        ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
        ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
        pos.y += ImGui::GetTextLineHeight() * 1.5f;
        //}}}
        if (mSelect == eSelectedFull) {
          //{{{  draw ptsFromStart bottomRight
          string ptsFromStartString = utils::getPtsString (mService.getPtsFromStart());
          pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
          ImGui::SetCursorPos (pos);
          ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsFromStartString.c_str());
          ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
          ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsFromStartString.c_str());
          }
          //}}}
        ImGui::PopFont();

        ImVec2 viewSubSize = mSize - ImVec2(0.f, ImGui::GetTextLineHeight() *
                                                   ((layoutPos.y + (layoutScale/2.f) >= 0.99f) ? 3.f : 1.5f));
        if (tellyApp.getOptions()->mShowEpg) {
          ImGui::SetCursorPos ({0.f, ImGui::GetTextLineHeight() * 1.5f});
          ImGui::BeginChild (fmt::format ("epg##{}", mService.getSid()).c_str(), viewSubSize,
                             ImGuiChildFlags_None,
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
          //{{{  draw epg
          ImVec2 posEpg = {0.25f,0.f};
          for (auto& epgItem : mService.getTodayEpg()) {
            auto time = epgItem.first;
            if (enabled && (mSelect != eSelectedFull))
              time += epgItem.second->getDuration();
            if (time > transportStream.getNowTdt()) {
              // draw epgItem title
              string epgTitle = date::format ("%H:%M ", date::floor<chrono::seconds>(epgItem.first)) +
                                       epgItem.second->getTitle();

              // drop shadow epg
              ImGui::SetCursorPos (posEpg);
              ImGui::TextColored ({0.f,0.f,0.f,1.f}, epgTitle.c_str());
              ImGui::SetCursorPos (posEpg - ImVec2(2.f,2.f));
              ImGui::TextColored ({1.f, 1.f,1.f,1.f}, epgTitle.c_str());

              posEpg.y += ImGui::GetTextLineHeight();
              }
            }
          //}}}
          }

        // invisbleButton over view sub area
        ImGui::SetCursorPos ({0.f,0.f});
        if (ImGui::InvisibleButton (fmt::format ("viewBox##{}", mService.getSid()).c_str(), viewSubSize)) {
          //{{{  hit view, select action
          if (!mService.getStream (cStream::eVideo).isEnabled()) {
            mService.enableStream (cStream::eVideo);
            mService.enableStream (cStream::eAudio);
            mService.enableStream (cStream::eSubtitle);
            }
          else if (mSelect == eUnselected)
            mSelect = eSelected;
          else if (mSelect == eSelected)
            mSelect = eSelectedFull;
          else if (mSelect == eSelectedFull)
            mSelect = eSelected;

          result = true;
          }
          //}}}
        if (tellyApp.getOptions()->mShowEpg)
          ImGui::EndChild();

        ImGui::EndChild();
        return result;
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
      void draw (cTellyApp& tellyApp, cTransportStream& transportStream) {

        // create shaders, firsttime we see graphics interface
        if (!mVideoShader)
          mVideoShader = tellyApp.getGraphics().createTextureShader (cTexture::eYuv420);
        if (!mSubtitleShader)
          mSubtitleShader = tellyApp.getGraphics().createTextureShader (cTexture::eRgba);

        // update viewMap from enabled services, take care to reuse cView's
        for (auto& pair : transportStream.getServiceMap()) {
          // find service sid in viewMap
          auto it = mViewMap.find (pair.first);
          if (it == mViewMap.end()) {
            cTransportStream::cService& service = pair.second;
            if (service.getStream (cStream::eVideo).isDefined())
              // enabled and not found, add service to viewMap
              mViewMap.emplace (service.getSid(), cView (service));
            }
          //else if (!service.getStream (eVideo).isEnabled())
            // found, but not enabled, remove service from viewMap
           // mViewMap.erase (it);
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
            if (view.second.draw (tellyApp, transportStream,
                                  selectedFull,  viewIndex, selectedFull ? 1 : mViewMap.size(),
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

    private:
      map <uint16_t, cView> mViewMap;
      cTextureShader* mVideoShader = nullptr;
      cTextureShader* mSubtitleShader = nullptr;
      };
    //}}}

    //{{{
    void hitTab (cTellyApp& tellyApp, uint8_t tabIndex) {
      mTabIndex = (mTabIndex == tabIndex) ? (uint8_t)eNone : tabIndex;
      tellyApp.setShowEpg (mTabIndex == eEpg);
      }
    //}}}
    //{{{
    void keyboard (cTellyApp& tellyApp) {

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
        { false, false,  false, ImGuiKey_F,          [this,&tellyApp]{ tellyApp.getPlatform().toggleFullScreen(); }},
        { false, false,  false, ImGuiKey_S,          [this,&tellyApp]{ tellyApp.toggleShowSubtitle(); }},
        { false, false,  false, ImGuiKey_L,          [this,&tellyApp]{ tellyApp.toggleShowMotionVectors(); }},
        { false, false,  false, ImGuiKey_P,          [this,&tellyApp]{ hitTab (tellyApp, ePids); }},
        { false, false,  false, ImGuiKey_E,          [this,&tellyApp]{ hitTab (tellyApp, eEpg); }},
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

      // queued keys
      //for (int i = 0; i < ImGui::GetIO().InputQueueCharacters.Size; i++) {
      //  ImWchar ch = ImGui::GetIO().InputQueueCharacters[i];
      //  cLog::log (LOGINFO, fmt::format ("enter {:4x} {} {} {}", ch, altKey, controlKey, shiftKey));
      //  }
      ImGui::GetIO().InputQueueCharacters.resize (0);
      }
    //}}}

    //{{{
    void drawPids (cTransportStream& transportStream) {
    // draw pids

      ImGui::SetCursorPos ({0.f,0.f});

      // calc error number width
      int errorChars = 1;
      while (transportStream.getNumErrors() > pow (10, errorChars))
        errorChars++;

      int prevSid = 0;
      for (auto& pidInfoItem : transportStream.getPidInfoMap()) {
        // iterate for pidInfo
        cTransportStream::cPidInfo& pidInfo = pidInfoItem.second;

        // draw separator, crude test for new service, fails sometimes
        if ((pidInfo.getSid() != prevSid) && (pidInfo.getStreamType() != 5) && (pidInfo.getStreamType() != 11))
          ImGui::Separator();

        // draw pid label
        ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {} {}",
                                pidInfo.mPackets, mPacketChars, pidInfo.mErrors, errorChars, pidInfo.getPid(),
                                utils::getFullPtsString (pidInfo.getPts()),
                                utils::getFullPtsString (pidInfo.getDts()),
                                pidInfo.getPidName()).c_str());

        // draw stream bar
        ImGui::SameLine();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        mMaxPidPackets = max (mMaxPidPackets, pidInfo.mPackets);
        float frac = pidInfo.mPackets / float(mMaxPidPackets);
        ImVec2 posTo = {pos.x + (frac * (ImGui::GetWindowWidth() - pos.x - ImGui::GetTextLineHeight())),
                        pos.y + ImGui::GetTextLineHeight()};
        ImGui::GetWindowDrawList()->AddRectFilled (pos, posTo, 0xff00ffff);

        // draw stream label
        string info = pidInfo.getInfo();
        if ((pidInfo.getStreamType() == 0) && (pidInfo.getSid() != 0xFFFF))
          info = fmt::format ("{} ", pidInfo.getSid()) + info;
        ImGui::TextUnformatted (info.c_str());

        // adjust packet number width
        if (pidInfo.mPackets > pow (10, mPacketChars))
          mPacketChars++;

        prevSid = pidInfo.getSid();
        }
      }
    //}}}
    // vars
    uint8_t mTabIndex = eNone;
    cMultiView& mMultiView;

    // pidInfo tabs
    int64_t mMaxPidPackets = 0;
    size_t mPacketChars = 3;
    size_t mMaxNameChars = 3;
    size_t mMaxSidChars = 3;
    size_t mMaxPgmChars = 3;
    };
  //}}}
  }

// main
int main (int numArgs, char* args[]) {

  cTellyOptions* options = new cTellyOptions();
  //{{{  parse commandLine options
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) {
      // found a cApp option
      if (!options->mHasGui)
        options->mShowAllServices = false;
      }
    else if (param == "none")
      options->mShowAllServices = false;
    else if (param == "all")
      options->mRecordAllServices = true;
    else if (param == "noaudio")
      options->mHasAudio = false;
    else if (param == "sub")
      options->mShowSubtitle = true;
    else if (param == "motion")
      options->mHasMotionVectors = true;
    else {
      // assume filename
      options->mFileName = param;

      // look for named multiplex
      for (auto& multiplex : kDvbMultiplexes) {
        if (param == multiplex.mName) {
          // found named multiplex
          options->mMultiplex = multiplex;
          options->mFileName = "";
          break;
          }
        }
      }
    }
  //}}}
  options->mRecordRoot = kRootDir;

  // log
  cLog::init (options->mLogLevel);
  cLog::log (LOGNOTICE, fmt::format ("telly all noaudio {} {}",
                                     (dynamic_cast<cApp::cOptions*>(options))->cApp::cOptions::getString(),
                                     options->cTellyOptions::getString()));

  cTellyApp tellyApp (options, new cTellyUI());
  if (options->mFileName.empty()) {
    options->mIsLive = true;
    if (options->mHasGui) {
      tellyApp.liveDvbSourceThread (options->mMultiplex, options);
      tellyApp.mainUILoop();
      }
    else
      tellyApp.liveDvbSource (options->mMultiplex, options);
    }
  else if (options->mFileName.substr (options->mFileName.size() - 3, 3) == ".ts") {
    tellyApp.fileSource (options->mFileName, options);
    tellyApp.mainUILoop();
    }
  else
    cLog::log (LOGERROR, fmt::format ("file not .ts {}", options->mFileName));

  return EXIT_SUCCESS;
  }

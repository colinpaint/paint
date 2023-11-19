// tellyApp.cpp - imgui telly main, app, UI
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// utils
#include "../common/date.h"
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"
#include "fmt/format.h"

// dvb
#include "../dvb/cDvbStream.h"
#include "../dvb/cVideoRender.h"
#include "../dvb/cVideoFrame.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cAudioFrame.h"
#include "../dvb/cSubtitleRender.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../app/myImgui.h"
#include "../app/cUI.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

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
  const vector <cDvbMultiplex> kMultiplexes = {
      { "file", false, 0, {}, {} },  // dummy multiplex for file

      { "hd", 626000000, false,
        { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
        { "bbc1hd",        "bbc2hd",     "bbc3hd",       "bbc4hd",      "itv1hd",  "chn4hd",       "chn5hd" }
      },

      { "itv", 650000000, false,
        { "ITV1",  "ITV2", "ITV3", "ITV4", "Channel 4", "Channel 4+1", "More 4", "Film4" , "E4", "Channel 5" },
        { "itv1", "itv2", "itv3", "itv4", "chn4"     , "c4+1",        "more4",  "film4",  "e4", "chn5" }
      },

      { "bbc", 674000000, false,
        { "BBC ONE S West", "BBC TWO", "BBC FOUR" },
        { "bbc1",           "bbc2",    "bbc4" }
      }
    };
  //}}}

  //{{{
  class cFramesView {
  public:
    cFramesView() {}
    ~cFramesView() = default;

    //{{{
    void draw (cAudioRender& audioRender, cVideoRender& videoRender, int64_t playerPts) {

      ImVec2 pos = { ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(),
                     ImGui::GetCursorScreenPos().y + ImGui::GetWindowHeight() - (2.f * ImGui::GetTextLineHeight()) };

      drawAudioFramePower (audioRender.findAudioFrameFromPts (playerPts), pos);

      pos.x -= ImGui::GetWindowWidth() * 0.5f;

      // draw playPts centre bar
      ImGui::GetWindowDrawList()->AddRectFilled (
        {pos.x, pos.y - mVideoLines * ImGui::GetTextLineHeight()},
        {pos.x + 1.f, pos.y + mAudioLines * ImGui::GetTextLineHeight()},
        0xffc0c0c0);

      float ptsScale = mPixelsPerVideoFrame / videoRender.getPtsDuration();

      { // lock video during iterate
      shared_lock<shared_mutex> lock (videoRender.getSharedMutex());
      for (auto& frame : videoRender.getFrames()) {
        //{{{  draw video frames
        cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame.second);
        float offset1 = (videoFrame->mPts - playerPts) * ptsScale;
        float offset2 = offset1 + (videoFrame->mPtsDuration * ptsScale) - 1.f;

        // pesSize I white / P purple / B blue - ABGR color
        ImGui::GetWindowDrawList()->AddRectFilled (
          {pos.x + offset1, pos.y},
          {pos.x + offset2, pos.y - addValue ((float)videoFrame->mPesSize, mMaxPesSize, mMaxDisplayPesSize, mVideoLines)},
          (videoFrame->mFrameType == 'I') ? 0xffffffff : (videoFrame->mFrameType == 'P') ? 0xffFF40ff : 0xffFF4040);
        }
        //}}}
      for (auto frame : videoRender.getFreeFrames()) {
        //{{{  draw free video frames
        cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
        float offset1 = (videoFrame->mPts - playerPts) * ptsScale;
        float offset2 = offset1 + (videoFrame->mPtsDuration * ptsScale) - 1.f;

        ImGui::GetWindowDrawList()->AddRectFilled (
          {pos.x + offset1, pos.y},
          {pos.x + offset2, pos.y - addValue ((float)videoFrame->mPesSize, mMaxPesSize, mMaxDisplayPesSize, mVideoLines)},
          0xFF808080);
        }
        //}}}
      }

      { // lock audio during iterate
      shared_lock<shared_mutex> lock (audioRender.getSharedMutex());
      for (auto& frame : audioRender.getFrames()) {
        //{{{  draw audio frames
        cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame.second);
        float offset1 = (audioFrame->mPts - playerPts) * ptsScale;
        float offset2 = offset1 + (audioFrame->mPtsDuration * ptsScale) - 1.f;
        ImGui::GetWindowDrawList()->AddRectFilled (
          {pos.x + offset1, pos.y + 1.f},
          {pos.x + offset2, pos.y + 1.f + addValue (audioFrame->mPeakValues[0], mMaxPower, mMaxDisplayPower, mAudioLines)},
          0xff00ffff);
        }
        //}}}
      for (auto frame : audioRender.getFreeFrames()) {
        //{{{  draw free audio frames
        cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);
        float offset1 = (audioFrame->mPts - playerPts) * ptsScale;
        float offset2 = offset1 + (audioFrame->mPtsDuration * ptsScale) - 1.f;

        ImGui::GetWindowDrawList()->AddRectFilled (
          {pos.x + offset1, pos.y + 1.f},
          {pos.x + offset2, pos.y + 1.f + addValue (audioFrame->mPeakValues[0], mMaxPower, mMaxDisplayPower, mAudioLines)},
          0xFF808080);
        }
        //}}}
      }

      // slowly track scaling back to max display values from max values
      agc (mMaxPesSize, mMaxDisplayPesSize, 250.f, 10000.f);
      agc (mMaxPower, mMaxDisplayPower, 250.f, 0.5f);
      }
    //}}}

  private:
    //{{{
    void drawAudioFramePower (cAudioFrame* audioFrame, ImVec2 pos) {

      if (audioFrame) {
        bool audio51 = audioFrame->mNumChannels == 6;
        array <size_t, 6> channelOrder;
        if (audio51)
          channelOrder = {4, 0, 2, 1, 5, 3};
        else
          channelOrder = {0, 1, 2, 3, 4, 5};

        float x = pos.x - (audio51 ? 5 : audioFrame->mNumChannels * mPixelsPerAudioChannel);
        pos.y += 1.f;
        float height = 8.f * ImGui::GetTextLineHeight();

        // draw channels
        for (size_t i = 0; i < (audio51 ? 5 : audioFrame->mNumChannels+1); i++) {
          ImGui::GetWindowDrawList()->AddRectFilled (
            { x, pos.y - (audioFrame->mPowerValues[channelOrder[i]] * height) },
            { x + mPixelsPerAudioChannel - 1.f, pos.y },
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
    //}}}

    //{{{
    float addValue (float value, float& maxValue, float& maxDisplayValue, float scale) {

      if (value > maxValue)
        maxValue = value;
      if (value > maxDisplayValue)
        maxDisplayValue = value;

      return scale * ImGui::GetTextLineHeight() * value / maxValue;
      }
    //}}}
    //{{{
    void agc (float& maxValue, float& maxDisplayValue, float revert, float minValue) {

      // slowly adjust scale to displayMaxValue
      if (maxDisplayValue < maxValue)
        maxValue -= (maxValue - maxDisplayValue) / revert;

      if (maxValue < minValue)
        maxValue = minValue;

      // reset displayed max value
      maxDisplayValue = 0.f;
      }
    //}}}

    //  vars
    const float mVideoLines = 2.f;
    const float mAudioLines = 1.f;
    const float mPixelsPerVideoFrame = 6.f;
    const float mPixelsPerAudioChannel = 6.f;

    float mMaxPower = 0.5f;
    float mMaxDisplayPower = 0.f;

    float mMaxPesSize = 0.f;
    float mMaxDisplayPesSize = 0.f;

    float mMaxQueueSize = 3.f;
    float mMaxDisplayQueueSize = 0.f;
    };
  //}}}
  //{{{
  class cView {
  public:
    cView (cDvbStream::cService& service, size_t index) : mService(service), mIndex(index) {}
    ~cView() = default;

    //{{{
    void draw (cGraphics& graphics, size_t numViews, size_t selectedIndex, float scale) {

      cVideoRender& videoRender = dynamic_cast<cVideoRender&> (mService.getRenderStream (eRenderVideo).getRender());

      // get playerPts from audioStream
      int64_t playerPts = 0;
      if (mService.getRenderStream (eRenderAudio).isEnabled()) {
        cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getRenderStream (eRenderAudio).getRender());

        // update playerPts from audioPlayer
        playerPts = audioRender.getPlayerPts();

        audioRender.setMute (mIndex != selectedIndex);
        if (mIndex == selectedIndex)
          mFramesView.draw (audioRender, videoRender, playerPts);
        }
      else
        playerPts = mService.getRenderStream (eRenderAudio).getPts();

      // draw videoFrame
      cVideoFrame* videoFrame = videoRender.getVideoNearestFrameFromPts (playerPts);
      if (videoFrame) {
        cVec2 pos = position (numViews);
        cVec2 scaledSize = { (scale * ImGui::GetWindowWidth()) / videoFrame->getWidth(),
                             (scale * ImGui::GetWindowHeight()) / videoFrame->getHeight() };
        cMat4x4 model;
        model.setTranslate ( {(ImGui::GetWindowWidth() * pos.x) - ((videoFrame->getWidth() / 2.f) * scaledSize.x),
                              (ImGui::GetWindowHeight() * pos.y) - ((videoFrame->getHeight() / 2.f) * scaledSize.y)} );
        model.size (scaledSize);
        mRect = videoRender.drawFrame (videoFrame, graphics, model, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

        ImGui::GetWindowDrawList()->AddRect (ImVec2((float)mRect.left, (float)mRect.top),
                                             ImVec2((float)mRect.right, (float)mRect.bottom), 0xff00ffff);

        videoRender.trimVideoBeforePts (playerPts - videoFrame->mPtsDuration);
        }
      }
    //}}}

  private:
    //{{{
    cVec2 position (size_t numViews) {

      switch (numViews) {
        //{{{
        case 2:
          switch (mIndex) {
            case 0: return { 1.f / 4.f, 0.5f };
            case 1: return { 3.f / 4.f, 0.5f };
            }
          break;
        //}}}
        case 3:
        //{{{
        case 4:
          switch (mIndex) {
            case 0: return { 1.f / 4.f, 3.f / 4.f };
            case 1: return { 3.f / 4.f, 3.f / 4.f };

            case 2: if (numViews == 3) // centre bottom
                      return { 0.5f, 1.f / 4.f };
                    else
                      return { 1.f / 4.f, 1.f / 4.f };
            case 3: return { 3.f / 4.f, 1.f / 4.f };
            }

          break;
        //}}}
        //{{{
        case 5:
          switch (mIndex) {
            case 0: return { 1.f / 6.f, 5.f / 6.f };
            case 1: return { 5.f / 6.f, 5.f / 6.f };
            case 2: return { 0.5f, 0.5f };
            case 3: return { 1.f / 6.f, 1.f / 6.f };
            case 4: return { 5.f / 6.f, 1.f / 6.f };
            }

          break;
        //}}}
        //{{{
        case 6:
          switch (mIndex) {
            case 0: return { 1.f / 6.f, 4.f / 6.f };
            case 1: return { 3.f / 6.f, 4.f / 6.f };
            case 2: return { 5.f / 6.f, 4.f / 6.f };

            case 3: return { 1.f / 6.f, 2.f / 6.f };
            case 4: return { 3.f / 6.f, 2.f / 6.f };
            case 5: return { 5.f / 6.f, 2.f / 6.f };
            }

          break;
        //}}}
        case 7:
        case 8:
        //{{{
        case 9:
          switch (mIndex) {
            case 0: return { 1.f / 6.f, 5.f / 6.f };
            case 1: return { 3.f / 6.f, 5.f / 6.f };
            case 2: return { 5.f / 6.f, 5.f / 6.f };

            case 3: return { 1.f / 6.f, 0.5f };
            case 4: return { 3.f / 6.f, 0.5f };
            case 5: return { 5.f / 6.f, 0.5f };

            case 6: if (numViews != 7)
                      return { 1.f / 6.f, 1.f / 6.f };
            case 7: return { 3.f / 6.f, 1.f / 6.f };
            case 8: return { 5.f / 6.f, 1.f / 6.f };
            }

          break;
        //}}}
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        //{{{
        case 16:
          switch (mIndex) {
            case  0: return { 1.f / 8.f, 7.f / 8.f };
            case  1: return { 3.f / 8.f, 7.f / 8.f };
            case  2: return { 5.f / 8.f, 7.f / 8.f };
            case  3: return { 7.f / 8.f, 7.f / 8.f };

            case  4: return { 1.f / 8.f, 5.f / 8.f };
            case  5: return { 3.f / 8.f, 5.f / 8.f };
            case  6: return { 5.f / 8.f, 5.f / 8.f };
            case  7: return { 7.f / 8.f, 5.f / 8.f };

            case  8: return { 1.f / 8.f, 3.f / 8.f };
            case  9: return { 3.f / 8.f, 3.f / 8.f };
            case 10: return { 5.f / 8.f, 3.f / 8.f };
            case 11: return { 7.f / 8.f, 3.f / 8.f };

            case 12: return { 1.f / 8.f, 1.f / 8.f };
            case 13: return { 3.f / 8.f, 1.f / 8.f };
            case 14: return { 5.f / 8.f, 1.f / 8.f };
            case 15: return { 7.f / 8.f, 1.f / 8.f };
            }

          break;
        //}}}
        }

      return { 0.5f, 0.5f };
      }
    //}}}

    cDvbStream::cService& mService;
    const size_t mIndex;

    cRect mRect;
    cFramesView mFramesView;
    };
  //}}}
  //{{{
  class cMultiView {
  public:
    cMultiView() {}
    ~cMultiView() = default;

    size_t getNumViews() const { return mViews.size(); }
    size_t getSelectedView() const { return mSelectedView; }

    //{{{
    void draw (cDvbStream& dvbStream, cGraphics& graphics, float scale) {

      mViews.clear();

      size_t index = 0;
      for (auto& pair : dvbStream.getServiceMap()) {
        cDvbStream::cService& service = pair.second;
        if (service.getRenderStream (eRenderVideo).isEnabled())
          mViews.push_back (cView (service, index++));
        }

      float viewScale = (getNumViews() <= 1) ? 1.f : ((getNumViews() <= 4) ? 0.5f : ((getNumViews() <= 9) ? 0.33f : 0.25f));

      for (auto& view : mViews)
        view.draw (graphics, getNumViews(), mSelectedView, scale * viewScale);
      }
    //}}}

  private:
    vector <cView> mViews;

    size_t mSelectedView = 0;
    };
  //}}}

  //{{{
  class cTellyApp : public cApp {
  public:
    //{{{
    cTellyApp (const cPoint& windowSize, bool fullScreen)
      : cApp("telly", windowSize, fullScreen, true) {}
    //}}}
    virtual ~cTellyApp() = default;

    bool hasDvbStream() { return mDvbStream; }
    cDvbStream& getDvbStream() { return *mDvbStream; }
    cMultiView& getMultiView() { return mMultiView; }

    //{{{
    void liveDvbSource (const cDvbMultiplex& dvbMultiplex, const string& recordRoot, bool showAllServices) {

      // create liveDvbSource from dvbMultiplex
      mDvbStream = new cDvbStream (dvbMultiplex, recordRoot, true, showAllServices, false);
      if (mDvbStream)
        mDvbStream->dvbSource();
      else
        cLog::log (LOGINFO, "cTellyApp::setLiveDvbSource - failed to create liveDvbSource");
      }
    //}}}
    //{{{
    void fileSource (const string& filename, bool showAllServices) {

      string resolvedFilename = cFileUtils::resolve (filename);

      // create fileSource, any channel
      mDvbStream = new cDvbStream (kMultiplexes[0], "", false, showAllServices, true);
      if (mDvbStream)
        mDvbStream->fileSource (resolvedFilename);
      else
        cLog::log (LOGINFO, fmt::format ("cTellyApp::fileSource - failed to create fileSource {} {}",
                                         filename, filename == resolvedFilename ? "" : resolvedFilename));
      }
    //}}}

    //{{{
    void drop (const vector<string>& dropItems) {
    // drop fileSource

      for (auto& item : dropItems) {
        cLog::log (LOGINFO, fmt::format ("cTellyApp::drop {}", item));
        fileSource (item, true);
        }
      }
    //}}}

  private:
    cDvbStream* mDvbStream = nullptr;
    cMultiView mMultiView;
    };
  //}}}
  //{{{
  class cTellyView {
  public:
    //{{{
    void draw (cApp& app) {

      cTellyApp& tellyApp = (cTellyApp&)app;
      cGraphics& graphics = tellyApp.getGraphics();
      cMultiView& multiView = tellyApp.getMultiView();

      graphics.clear (cPoint((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

      // draw multiView tellys as background
      if (tellyApp.hasDvbStream())
        multiView.draw (tellyApp.getDvbStream(), graphics, mScale);

      //{{{  draw tabs
      ImGui::SameLine();

      mTab = (eTab)interlockedButtons (kTabNames, (uint8_t)mTab, {0.f,0.f}, true);
      //}}}
      //{{{  draw fullScreen
      if (tellyApp.getPlatform().hasFullScreen()) {
        ImGui::SameLine();
        if (toggleButton ("full", tellyApp.getPlatform().getFullScreen()))
          tellyApp.getPlatform().toggleFullScreen();
        }
      //}}}
      //{{{  draw scale
      ImGui::SameLine();

      ImGui::SetNextItemWidth (4.f * ImGui::GetTextLineHeight());
      ImGui::DragFloat ("##scale", &mScale, 0.01f, 0.05f, 16.f, "scale%3.2f");
      //}}}
      //{{{  draw vsync
      ImGui::SameLine();

      if (tellyApp.getPlatform().hasVsync())
        if (toggleButton ("vsync", tellyApp.getPlatform().getVsync()))
          tellyApp.getPlatform().toggleVsync();
      //}}}
      //{{{  draw frameRate
      ImGui::SameLine();

      ImGui::TextUnformatted (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      //}}}
      //{{{  draw vertices:indices
      ImGui::SameLine();

      ImGui::TextUnformatted (fmt::format ("{}:{}",
                              ImGui::GetIO().MetricsRenderVertices,
                              ImGui::GetIO().MetricsRenderIndices/3).c_str());
      //}}}
      //{{{  draw overlap,history
      //ImGui::SameLine();
      //ImGui::SetNextItemWidth (4.f * ImGui::GetTextLineHeight());
      //ImGui::DragFloat ("##over", &mOverlap, 0.25f, 0.5f, 32.f, "over%3.1f");

      //ImGui::SameLine();
      //ImGui::SetNextItemWidth (3.f * ImGui::GetTextLineHeight());
      //ImGui::DragInt ("##hist", &mHistory, 0.25f, 0, 100, "h %d");
      //}}}

      if (tellyApp.hasDvbStream()) {
        cDvbStream& dvbStream = tellyApp.getDvbStream();
        if (dvbStream.hasTdtTime()) {
          //{{{  draw tdtTime
          ImGui::SameLine();

          ImGui::TextUnformatted (dvbStream.getTdtTimeString().c_str());
          }
          //}}}
        if (dvbStream.isFileSource()) {
          //{{{  draw filePos
          ImGui::SameLine();

          //ImGui::TextUnformatted (fmt::format ("{}k of {}k",
          //                                     dvbStream.getFilePos()/1000,
          //                                     dvbStream.getFileSize()/1000).c_str());
          ImGui::TextUnformatted (fmt::format ("{:4.3f}%", (100.f * dvbStream.getFilePos()) / dvbStream.getFileSize()).c_str());
          }
          //}}}
        else if (dvbStream.hasDvbSource()) {
          //{{{  draw dvbStream::dvbSource signal:errors
          ImGui::SameLine();

          ImGui::TextUnformatted (fmt::format ("{}:{}", dvbStream.getSignalString(), dvbStream.getErrorString()).c_str());
          }
          //}}}
        //{{{  draw dvbStream packet,errors
        ImGui::SameLine();

        ImGui::TextUnformatted (fmt::format ("{}:{}", dvbStream.getNumPackets(), dvbStream.getNumErrors()).c_str());
        //}}}

        // draw tab childWindow, monospaced font
        ImGui::PushFont (tellyApp.getMonoFont());
        ImGui::BeginChild ("tab", {0.f,0.f}, false,
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);
        switch (mTab) {
          case eTelly:    drawChannels (dvbStream, graphics); break;
          case eServices: drawServices (dvbStream, graphics); break;
          case ePids:     drawPidMap (dvbStream, graphics); break;
          case eRecorded: drawRecorded (dvbStream, graphics); break;
          }

        if (ImGui::IsMouseClicked (0)) {
          cLog::log (LOGINFO, fmt::format ("mouse {} {} {},{}",
                                           ImGui::IsMouseDragging (0), ImGui::IsMouseDown (0),
                                           ImGui::GetMousePos().x, ImGui::GetMousePos().y));
          }


        keyboard();

        ImGui::EndChild();
        ImGui::PopFont();
        }
      }
    //}}}

  private:
    enum eTab { eTelly, eServices, ePids, eRecorded };
    inline static const vector<string> kTabNames = { "telly", "services", "pids", "recorded" };
    //{{{
    void drawChannels (cDvbStream& dvbStream, cGraphics& graphics) {
      (void)graphics;

      // draw services/channels
      for (auto& pair : dvbStream.getServiceMap()) {
        cDvbStream::cService& service = pair.second;
        if (ImGui::Button (fmt::format ("{:{}s}", service.getChannelName(), mMaxNameChars).c_str()))
          service.toggleAll();

        if (service.getRenderStream (eRenderAudio).isDefined()) {
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{}{}",
            service.getRenderStream (eRenderAudio).isEnabled() ? "*":"", service.getNowTitleString()).c_str());
          }

        while (service.getChannelName().size() > mMaxNameChars)
          mMaxNameChars = service.getChannelName().size();
        }
      }
    //}}}
    //{{{
    void drawServices (cDvbStream& dvbStream, cGraphics& graphics) {

      for (auto& pair : dvbStream.getServiceMap()) {
        // iterate services
        cDvbStream::cService& service = pair.second;
        //{{{  update button maxChars
        while (service.getChannelName().size() > mMaxNameChars)
          mMaxNameChars = service.getChannelName().size();
        while (service.getSid() > pow (10, mMaxSidChars))
          mMaxSidChars++;
        while (service.getProgramPid() > pow (10, mMaxPgmChars))
          mMaxPgmChars++;

        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
          uint16_t pid = service.getRenderStream (eRenderType(renderType)).getPid();
          while (pid > pow (10, mPidMaxChars[renderType]))
            mPidMaxChars[renderType]++;
          }
        //}}}

        // draw channel name pgm,sid - sid ensures unique button name
        if (ImGui::Button (fmt::format ("{:{}s} {:{}d}:{:{}d}",
                           service.getChannelName(), mMaxNameChars,
                           service.getProgramPid(), mMaxPgmChars,
                           service.getSid(), mMaxSidChars).c_str()))
          service.toggleAll();

        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
         // iterate definedStreams
          cDvbStream::cStream& stream = service.getRenderStream (eRenderType(renderType));
          if (stream.isDefined()) {
            ImGui::SameLine();
            // draw definedStream button - sid ensuresd unique button name
            if (toggleButton (fmt::format ("{}{:{}d}:{}##{}",
                                           stream.getLabel(),
                                           stream.getPid(), mPidMaxChars[renderType], stream.getTypeName(),
                                           service.getSid()).c_str(), stream.isEnabled()))
             dvbStream.toggleStream (service, eRenderType(renderType));
            }
          }

        if (service.getChannelRecord()) {
          //{{{  draw record pathName
          ImGui::SameLine();
          ImGui::Button (fmt::format ("rec:{}##{}",
                                      service.getChannelRecordName(), service.getSid()).c_str());
          }
          //}}}

        // audio provides playPts
        int64_t playPts = 0;
        if (service.getRenderStream (eRenderAudio).isEnabled())
          playPts = dynamic_cast<cAudioRender&>(service.getRenderStream (eRenderAudio).getRender()).getPlayerPts();

        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
          // iterate enabledStreams, drawing
          cDvbStream::cStream& stream = service.getRenderStream (eRenderType(renderType));
          if (stream.isEnabled()) {
            switch (eRenderType(renderType)) {
              case eRenderVideo:
                drawVideoInfo (service.getSid(), stream.getRender(), graphics, playPts); break;

              case eRenderAudio:
              case eRenderDescription:
                drawAudioInfo (service.getSid(), stream.getRender(), graphics);  break;

              case eRenderSubtitle:
                drawSubtitle (service.getSid(), stream.getRender(), graphics);  break;

              default:;
              }
            }
          }
        }
      }
    //}}}
    //{{{
    void drawPidMap (cDvbStream& dvbStream, cGraphics& graphics) {
    // draw pids
      (void)graphics;

      // calc error number width
      int errorChars = 1;
      while (dvbStream.getNumErrors() > pow (10, errorChars))
        errorChars++;

      int prevSid = 0;
      for (auto& pidInfoItem : dvbStream.getPidInfoMap()) {
        // iterate for pidInfo
        cDvbStream::cPidInfo& pidInfo = pidInfoItem.second;

        // draw separator, crude test for new service, fails sometimes
        if ((pidInfo.getSid() != prevSid) && (pidInfo.getStreamType() != 5) && (pidInfo.getStreamType() != 11))
          ImGui::Separator();

        // draw pid label
        ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {} {}",
                                pidInfo.mPackets, mPacketChars, pidInfo.mErrors, errorChars, pidInfo.getPid(),
                                utils::getFullPtsString (pidInfo.getPts()),
                                utils::getFullPtsString (pidInfo.getDts()),
                                pidInfo.getTypeName()).c_str());

        // draw stream bar
        ImGui::SameLine();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        mMaxPidPackets = max (mMaxPidPackets, pidInfo.mPackets);
        float frac = pidInfo.mPackets / float(mMaxPidPackets);
        ImVec2 posTo = {pos.x + (frac * (ImGui::GetWindowWidth() - pos.x - ImGui::GetTextLineHeight())),
                        pos.y + ImGui::GetTextLineHeight()};
        ImGui::GetWindowDrawList()->AddRectFilled (pos, posTo, 0xff00ffff);

        // draw stream label
        string streamText = pidInfo.getInfoString();
        if ((pidInfo.getStreamType() == 0) && (pidInfo.getSid() != 0xFFFF))
          streamText = fmt::format ("{} ", pidInfo.getSid()) + streamText;
        ImGui::TextUnformatted (streamText.c_str());

        // adjust packet number width
        if (pidInfo.mPackets > pow (10, mPacketChars))
          mPacketChars++;

        prevSid = pidInfo.getSid();
        }
      }
    //}}}
    //{{{
    void drawRecorded (cDvbStream& dvbStream, cGraphics& graphics) {
      (void)graphics;

      for (auto& program : dvbStream.getRecordPrograms())
        ImGui::TextUnformatted (program.c_str());
      }
    //}}}

    //{{{
    void drawSubtitle (uint16_t sid, cRender& render, cGraphics& graphics) {
      (void)sid;

      cSubtitleRender& subtitleRender = dynamic_cast<cSubtitleRender&>(render);

      const float potSize = ImGui::GetTextLineHeight() / 2.f;
      size_t line = 0;
      while (line < subtitleRender.getNumLines()) {
        // draw clut color pots
        ImVec2 pos = ImGui::GetCursorScreenPos();
        for (size_t pot = 0; pot < subtitleRender.getImage (line).mColorLut.max_size(); pot++) {
          ImVec2 potPos {pos.x + (pot % 8) * potSize, pos.y + (pot / 8) * potSize};
          uint32_t color = subtitleRender.getImage (line).mColorLut[pot];
          ImGui::GetWindowDrawList()->AddRectFilled (
            potPos, {potPos.x + potSize - 1.f, potPos.y + potSize - 1.f}, color);
          }

        if (ImGui::InvisibleButton (fmt::format ("##subLog{}{}", sid, line).c_str(),
                                    {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
          subtitleRender.toggleLog();

        // draw position
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("{:3d},{:3d}", subtitleRender.getImage (line).mX,
                                                            subtitleRender.getImage (line).mY).c_str());

        // create subtitle line image
        if (!subtitleRender.getImage (line).mTexture) // create
          subtitleRender.getImage (line).mTexture = graphics.createTexture (
            cTexture::eRgba, {subtitleRender.getImage (line).mWidth, subtitleRender.getImage (line).mHeight});

        // update subtitle line image
        if (subtitleRender.getImage (line).mDirty)
          subtitleRender.getImage (line).mTexture->setPixels (&subtitleRender.getImage (line).mPixels, nullptr);
        subtitleRender.getImage (line).mDirty = false;

        // draw subtitle line image, scaled to fit line
        ImGui::SameLine();
        ImGui::Image ((void*)(intptr_t)subtitleRender.getImage (line).mTexture->getTextureId(),
                      { ImGui::GetTextLineHeight() *
                          subtitleRender.getImage (line).mWidth  / subtitleRender.getImage (line).mHeight,
                        ImGui::GetTextLineHeight() });
        line++;
        }

      //{{{  pad with invisible lines to highwaterMark
      while (line < subtitleRender.getHighWatermark()) {
        if (ImGui::InvisibleButton (fmt::format ("##line{}", line).c_str(),
                                    {ImGui::GetWindowWidth() - ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
          subtitleRender.toggleLog();
        line++;
        }
      //}}}
      drawMiniLog (subtitleRender.getLog());
      }
    //}}}
    //{{{
    void drawAudioInfo (uint16_t sid, cRender& render, cGraphics& graphics) {
      (void)sid;
      (void)graphics;

      cAudioRender& audioRender = dynamic_cast<cAudioRender&>(render);

      ImGui::TextUnformatted (audioRender.getInfoString().c_str());

      if (ImGui::InvisibleButton (fmt::format ("##audLog{}", sid).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        audioRender.toggleLog();

      drawMiniLog (audioRender.getLog());
      }
    //}}}
    //{{{
    void drawVideoInfo (uint16_t sid, cRender& render, cGraphics& graphics, int64_t playPts) {
      (void)sid;
      (void)graphics;
      (void)playPts;

      cVideoRender& videoRender = dynamic_cast<cVideoRender&>(render);
      ImGui::TextUnformatted (videoRender.getInfoString().c_str());

      if (ImGui::InvisibleButton (fmt::format ("##vidLog{}", sid).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        videoRender.toggleLog();

      drawMiniLog (videoRender.getLog());
      }
    //}}}

    // not yet
    //{{{
    void undo() {
      cLog::log (LOGINFO, "undo");
      }
    //}}}
    //{{{
    void redo() {
      cLog::log (LOGINFO, "redo");
      }
    //}}}
    //{{{
    void toggleFullScreen() {
      cLog::log (LOGINFO, "toggleFullScreen");
      }
    //}}}

    //{{{
    void keyboard() {

      ImGui::GetIO().WantTextInput = true;
      ImGui::GetIO().WantCaptureKeyboard = false;

      bool altKeyPressed = ImGui::GetIO().KeyAlt;
      bool ctrlKeyPressed = ImGui::GetIO().KeyCtrl;
      bool shiftKeyPressed = ImGui::GetIO().KeyShift;

      for (int i = 0; i < ImGui::GetIO().InputQueueCharacters.Size; i++) {
        ImWchar ch = ImGui::GetIO().InputQueueCharacters[i];
        cLog::log (LOGINFO, fmt::format ("enter {:4x} {} {} {}",
                                         ch,altKeyPressed, ctrlKeyPressed, shiftKeyPressed));
        }
      ImGui::GetIO().InputQueueCharacters.resize (0);
      }
    //}}}

    // vars
    eTab mTab = eTelly;

    int64_t mMaxPidPackets = 0;
    size_t mPacketChars = 3;
    size_t mMaxNameChars = 3;
    size_t mMaxSidChars = 3;
    size_t mMaxPgmChars = 3;

    array <size_t, 4> mPidMaxChars = { 3 };
    //array <size_t, 4> mPidMaxChars = { 3 };

    float mScale = 1.f;
    float mOverlap = 4.f;
    int mHistory = 0;
    };
  //}}}
  //{{{
  // cTellyUI
  class cTellyUI : public cUI {
  public:
    cTellyUI (const string& name) : cUI(name) {}
    virtual ~cTellyUI() = default;

    //{{{
    void addToDrawList (cApp& app) final {
    // draw into window

      ImGui::SetNextWindowPos (ImVec2(0,0));
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

      ImGui::Begin ("telly", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
      mTellyView.draw (app);
      ImGui::End();
      }
    //}}}

  private:
    // self registration
    static cUI* create (const string& className) { return new cTellyUI (className); }

    // static var self registration trick
    inline static const bool mRegistered = registerClass ("telly", &create);

    // vars
    cTellyView mTellyView;
    };
  //}}}
  }

// main
int main (int numArgs, char* args[]) {

  // params
  bool recordAll = false;
  bool showAllServices = true;
  bool fullScreen = false;
  eLogLevel logLevel = LOGINFO;
  cDvbMultiplex selectedMultiplex = kMultiplexes[1];
  string filename;
  //{{{  parse commandLine to params
  // parse params
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "all")
      recordAll = true;
    else if (param == "simple")
      showAllServices = false;
    else if (param == "full")
      fullScreen = true;
    else if (param == "log1")
      logLevel = LOGINFO1;
    else if (param == "log2")
      logLevel = LOGINFO2;
    else if (param == "log3")
      logLevel = LOGINFO3;
    else {
      // assume filename
      filename = param;

      // look for named multiplex
      for (auto& multiplex : kMultiplexes) {
        if (param == multiplex.mName) {
          // found named multiplex
          selectedMultiplex = multiplex;
          filename = "";
          break;
          }
        }
      }
    }

  selectedMultiplex.mRecordAll = recordAll;
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, "tellyApp - all simple full log1 log2 log3 multiplexName filename");

  // list static registered UI classes
  cUI::listRegisteredClasses();

  // app
  cTellyApp tellyApp ({1920/2, 1080/2}, fullScreen);
  tellyApp.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
  tellyApp.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 18.f));
  if (filename.empty()) {
    cLog::log (LOGINFO, fmt::format ("- using multiplex {}", selectedMultiplex.mName));
    tellyApp.liveDvbSource (selectedMultiplex, kRootDir, showAllServices);
    }
  else
    tellyApp.fileSource (filename, showAllServices);
  tellyApp.mainUILoop();

  return EXIT_SUCCESS;
  }

// telly.cpp - imgui telly main, app, UI
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

// UI
#include "../app/myImgui.h"
#include "../ui/cUI.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

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

using namespace std;
//}}}

namespace {
  #ifdef _WIN32
    const string kRecordRoot = "/tv/";
  #else
    const string kRecordRoot = "/home/pi/tv/";
  #endif
  //{{{
  const cDvbMultiplex kFileDvbMultiplex = {
    "hd",
    0,
    { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
    { "bbc1hd",        "bbc2hd",     "bbc3hd",       "bbc4hd",      "itv1hd",  "chn4hd",       "chn5hd" },
    false,
    };
  //}}}
  //{{{
  const vector <cDvbMultiplex> kDvbMultiplexes = {
    { "hd",
      626000000,
      { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
      { "bbc1hd",        "bbc2hd",     "bbc3hd",       "bbc4hd",      "itv1hd",  "chn4hd",       "chn5hd" },
      false,
    },

    { "itv",
      650000000,
      { "ITV1",  "ITV2", "ITV3", "ITV4", "Channel 4", "Channel 4+1", "More 4", "Film4" , "E4", "Channel 5" },
      { "itv1", "itv2", "itv3", "itv4", "chn4"     , "c4+1",        "more4",  "film4",  "e4", "chn5" },
      false,
    },

    { "bbc",
      674000000,
      { "BBC ONE S West", "BBC TWO", "BBC FOUR" },
      { "bbc1",           "bbc2",    "bbc4" },
      false,
    },

    { "allhd",
      626000000,
      { "" },
      { "" },
      true,
    },

    { "allitv",
      650000000,
      { "" },
      { "" },
      true,
    },

    { "allbbc",
      674000000,
      { "" },
      { "" },
      true,
    },
    };
  //}}}
  }

//{{{
class cTellyApp : public cApp {
public:
  //{{{
  cTellyApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp("telly", windowSize, fullScreen, vsync) {}
  //}}}
  virtual ~cTellyApp() = default;

  cDvbStream* getDvbStream() { return mDvbStream; }

  //{{{
  bool setSource (const string& filename, const string& recordRoot, const cDvbMultiplex& dvbMultiplex,
                  bool renderFirstService) {

    if (filename.empty()) {
    // create dvbSource from dvbMultiplex
      mDvbStream = new cDvbStream (dvbMultiplex, recordRoot, renderFirstService);
      if (!mDvbStream)
        return false;
      mDvbStream->dvbSource (true);
      return true;
      }

    else {
      // create fileSource, with dummy all multiplex
      mDvbStream = new cDvbStream (kFileDvbMultiplex,  "", true);
      if (!mDvbStream)
        return false;

      cFileUtils::resolve (filename);
      mDvbStream->fileSource (true, filename);

      return true;
      }
    }
  //}}}
  //{{{
  void drop (const vector<string>& dropItems) {
  // drop fileSource

    for (auto& item : dropItems) {
      string filename = cFileUtils::resolve (item);
      setSource (filename, "", kFileDvbMultiplex, true);
      cLog::log (LOGINFO, fmt::format ("cTellyApp::drop {}", filename));
      }
    }
  //}}}

private:
  cDvbStream* mDvbStream = nullptr;
  };
//}}}
//{{{
class cTellyView {
public:
  //{{{
  void draw (cTellyApp& tellyApp) {

    cGraphics& graphics = tellyApp.getGraphics();
    graphics.clear (cPoint((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

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


    if (tellyApp.getDvbStream()) {
      cDvbStream& dvbStream = *tellyApp.getDvbStream();
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
        //{{{
        case eTelly:
          drawTelly (dvbStream, graphics);
          drawChannels (dvbStream, graphics);
          break;
        //}}}
        //{{{
        case eServices:
          drawTelly (dvbStream, graphics);
          drawServices (dvbStream, graphics);
          break;
        //}}}
        //{{{
        case ePids:
          drawTelly (dvbStream, graphics);
          drawPids (dvbStream, graphics);
          break;
        //}}}
        //{{{
        case eRecorded:
          drawTelly (dvbStream, graphics);
          drawRecorded (dvbStream, graphics);
          break;
        //}}}
        //{{{
        case eHistory:
          drawHistory (dvbStream, graphics);
          break;
        //}}}
        }

      keyboard();

      ImGui::EndChild();
      ImGui::PopFont();
      }
    }
  //}}}

private:
  enum eTab { eTelly, eServices, ePids, eRecorded, eHistory };
  inline static const vector<string> kTabNames = { "telly", "services", "pids", "recorded" }; // "history"};
  //{{{
  class cFramesGraphic {
  public:
    cFramesGraphic (float videoLines, float audioLines, float pixelsPerVideoFrame, float pixelsPerAudioChannel)
      : mVideoLines(videoLines), mAudioLines(audioLines),
        mPixelsPerVideoFrame(pixelsPerVideoFrame), mPixelsPerAudioChannel(pixelsPerAudioChannel) {}

    //{{{
    void draw (cAudioRender& audioRender, cVideoRender& videoRender, int64_t playPts) {

      ImVec2 pos = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(),
                    ImGui::GetCursorScreenPos().y + ImGui::GetWindowHeight() - ImGui::GetTextLineHeight()};

      drawAudioPower (audioRender, playPts, pos);

      pos.x -= ImGui::GetWindowWidth() / 2.f;

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
        float offset1 = (videoFrame->mPts - playPts) * ptsScale;
        float offset2 = offset1 + (videoFrame->mPtsDuration * ptsScale) - 1.f;

        // pesSize I white / P yellow / B green - ARGB color
        ImGui::GetWindowDrawList()->AddRectFilled (
          {pos.x + offset1, pos.y},
          {pos.x + offset2, pos.y - addValue ((float)videoFrame->mPesSize, mMaxPesSize, mMaxDisplayPesSize, mVideoLines)},
          (videoFrame->mFrameType == 'I') ? 0xffffffff : (videoFrame->mFrameType == 'P') ? 0xff00ffff : 0xff00ff00);

        // grey decodeTime
        //ImGui::GetWindowDrawList()->AddRectFilled (
        //  {pos.x + offset1, pos.y},
        //  {pos.x + offset2, pos.y - addValue ((float)videoFrame->mTimes[0], mMaxDecodeTime, mMaxDisplayDecodeTime, mVideoLines)},
        //  0x60ffffff);

        // magenta queueSize in trailing gap
        //ImGui::GetWindowDrawList()->AddRectFilled (
        //  {pos.x + offset1, pos.y},
        //  {pos.x + offset1 + 1.f, pos.y - addValue ((float)videoFrame->mQueueSize, mMaxQueueSize, mMaxDisplayQueueSize, mVideoLines - 1.f)},
        //  0xffff00ff);
        }
        //}}}
      for (auto frame : videoRender.getFreeFrames()) {
        //{{{  draw free video frames
        cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
        float offset1 = (videoFrame->mPts - playPts) * ptsScale;
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
        float offset1 = (audioFrame->mPts - playPts) * ptsScale;
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
        float offset1 = (audioFrame->mPts - playPts) * ptsScale;
        float offset2 = offset1 + (audioFrame->mPtsDuration * ptsScale) - 1.f;

        ImGui::GetWindowDrawList()->AddRectFilled (
          {pos.x + offset1, pos.y + 1.f},
          {pos.x + offset2, pos.y + 1.f + addValue (audioFrame->mPeakValues[0], mMaxPower, mMaxDisplayPower, mAudioLines)},
          0xFF808080);
        }
        //}}}
      }

      // slowly track scaling back to max display values from max values
      agc (mMaxPower, mMaxDisplayPower, 250.f, 0.5f);
      agc (mMaxPesSize, mMaxDisplayPesSize, 250.f, 10000.f);
      agc (mMaxDecodeTime, mMaxDisplayDecodeTime, 250.f, 1000.f);
      agc (mMaxQueueSize, mMaxDisplayQueueSize, 250.f, 4.f);
      }
    //}}}

  private:
    //{{{
    void drawAudioPower (cAudioRender& audio, int64_t playerPts, ImVec2 pos) {

      cAudioFrame* audioFrame = audio.findAudioFrameFromPts (playerPts);
      if (audioFrame) {
        pos.y += 1.f;
        float height = 8.f * ImGui::GetTextLineHeight();

        if (audioFrame->mNumChannels == 6) {
          // 5.1
          float x = pos.x - (5 * mPixelsPerAudioChannel);

          // draw channels reordered
          for (size_t i = 0; i < 5; i++) {
            ImGui::GetWindowDrawList()->AddRectFilled (
              {x, pos.y - (audioFrame->mPowerValues[kChannelOrder[i]] * height)},
              {x + mPixelsPerAudioChannel - 1.f, pos.y},
              0xff00ff00);
            x += mPixelsPerAudioChannel;
            }

          // draw woofer as red
          ImGui::GetWindowDrawList()->AddRectFilled (
            {pos.x - (5 * mPixelsPerAudioChannel), pos.y - (audioFrame->mPowerValues[kChannelOrder[5]] * height)},
            {pos.x - 1.f, pos.y},
            0x800000ff);
          }

        else  {
          // other - draw channels left to right
          float width = mPixelsPerAudioChannel * audioFrame->mNumChannels;
          for (size_t i = 0; i < audioFrame->mNumChannels; i++)
            ImGui::GetWindowDrawList()->AddRectFilled (
              {pos.x - width + (i * mPixelsPerAudioChannel), pos.y - (audioFrame->mPowerValues[i] * height)},
              {pos.x - width + ((i+1) * mPixelsPerAudioChannel) - 1.f, pos.y},
              0xff00ff00);
          }
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

    inline static const array <size_t, 6> kChannelOrder = {4,0,2,1,5,3};

    //  vars
    const float mVideoLines;
    const float mAudioLines;
    const float mPixelsPerVideoFrame;
    const float mPixelsPerAudioChannel;

    float mMaxPower = 0.5f;
    float mMaxDisplayPower = 0.f;

    float mMaxPesSize = 0.f;
    float mMaxDisplayPesSize = 0.f;

    float mMaxQueueSize = 3.f;
    float mMaxDisplayQueueSize = 0.f;

    float mMaxDecodeTime = 0.f;
    float mMaxDisplayDecodeTime = 0.f;
    };
  //}}}
  //{{{
  class cVideoFrameDraw {
  public:
    cVideoFrameDraw (cVideoFrame* videoFrame, float offset) : mVideoFrame(videoFrame), mOffset(offset) {}

    cVideoFrame* mVideoFrame;
    float mOffset;
    };
  //}}}

  //{{{
  void drawChannels (cDvbStream& dvbStream, cGraphics& graphics) {
    (void)graphics;

    for (auto& pair : dvbStream.getServiceMap()) {
      //  draw services/channels
      cDvbStream::cService& service = pair.second;
      if (ImGui::Button (fmt::format ("{:{}s}", service.getChannelName(), mMaxNameChars).c_str()))
        service.toggleAll();

      if (service.getStream (cDvbStream::eAudio).isDefined()) {
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("{}{}",
          service.getStream (cDvbStream::eAudio).isEnabled() ? "*":"", service.getNowTitleString()).c_str());
        //ImGui::TextUnformatted (fmt::format ("{} {}", audio.getInfo(), videoRender.getInfo()).c_str());
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

      for (size_t streamType = cDvbStream::eVideo; streamType <= cDvbStream::eSubtitle; streamType++) {
        uint16_t pid = service.getStream (streamType).getPid();
        while (pid > pow (10, mPidMaxChars[streamType]))
          mPidMaxChars[streamType]++;
        }
      //}}}

      // draw channel name pgm,sid - sid ensures unique button name
      if (ImGui::Button (fmt::format ("{:{}s} {:{}d}:{:{}d}",
                         service.getChannelName(), mMaxNameChars,
                         service.getProgramPid(), mMaxPgmChars,
                         service.getSid(), mMaxSidChars).c_str()))
        service.toggleAll();

      for (size_t streamType = cDvbStream::eVideo; streamType <= cDvbStream::eSubtitle; streamType++) {
       // iterate definedStreams
        cDvbStream::cStream& stream = service.getStream (streamType);
        if (stream.isDefined()) {
          ImGui::SameLine();
          // draw definedStream button - sid ensuresd unique button name
          if (toggleButton (fmt::format ("{}{:{}d}:{}##{}",
                                         stream.getLabel(),
                                         stream.getPid(), mPidMaxChars[streamType], stream.getTypeName(),
                                         service.getSid()).c_str(), stream.isEnabled()))
           dvbStream.toggleStream (service, streamType);
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
      if (service.getStream (cDvbStream::eAudio).isEnabled())
        playPts = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAudio).getRender()).getPlayerPts();

      for (size_t streamType = cDvbStream::eVideo; streamType <= cDvbStream::eSubtitle; streamType++) {
        // iterate enabledStreams, drawing
        cDvbStream::cStream& stream = service.getStream (streamType);
        if (stream.isEnabled()) {
          switch (streamType) {
            case cDvbStream::eVideo:
              drawVideoInfo (service.getSid(), stream.getRender(), graphics, playPts); break;

            case cDvbStream::eAudio:
            case cDvbStream::eDescription:
              drawAudioInfo (service.getSid(), stream.getRender(), graphics);  break;

            case cDvbStream::eSubtitle:
              drawSubtitle (service.getSid(), stream.getRender(), graphics);  break;

            default:;
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void drawPids (cDvbStream& dvbStream, cGraphics& graphics) {
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
  void drawHistory (cDvbStream& dvbStream, cGraphics& graphics) {

    cVec2 windowSize = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};

    for (auto& pair : dvbStream.getServiceMap()) {
      cDvbStream::cService& service = pair.second;
      if (!service.getStream (cDvbStream::eVideo).isEnabled())
        continue;

      cVideoRender& videoRender = dynamic_cast<cVideoRender&>(service.getStream (cDvbStream::eVideo).getRender());
      cPoint videoSize = {videoRender.getWidth(), videoRender.getHeight()};

      // playPts and draw framesGraphic
      int64_t playPts = service.getStream (cDvbStream::eAudio).getPts();
      if (service.getStream (cDvbStream::eAudio).isEnabled()) {
        cAudioRender& audioRender = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAudio).getRender());
        playPts = audioRender.getPlayerPts();

        mFramesGraphic.draw (audioRender, videoRender, playPts);
        }

      // draw telly history pic
      cVideoFrame* videoFrame = videoRender.getVideoFrameFromPts (playPts);
      if (videoFrame) {
        // form draw list
        deque <cVideoFrameDraw> mVideoFramesDraws;
        if (mScale >= 1.f)
          mVideoFramesDraws.push_back ({videoFrame, 0.f});
        else {
          // locked
          shared_lock<shared_mutex> lock (videoRender.getSharedMutex());
          for (auto& frame : videoRender.getFrames()) {
            videoFrame = dynamic_cast<cVideoFrame*>(frame.second);
            int64_t offset = (videoFrame->mPts / videoFrame->mPtsDuration) - (playPts / videoFrame->mPtsDuration);
            if (offset <= 0) // draw last
              mVideoFramesDraws.push_back ({videoFrame, mOverlap * offset});
            else // draw in reverse order
              mVideoFramesDraws.push_front ({videoFrame, mOverlap * offset});
            }
          }

        // setup draw quad, shader
        if (!mQuad)
          mQuad = graphics.createQuad (videoSize);
        cMat4x4 model;
        cMat4x4 orthoProjection (0.f,static_cast<float>(windowSize.x) , 0.f, static_cast<float>(windowSize.y), -1.f, 1.f);
        cVec2 size = {mScale * windowSize.x / videoSize.x, mScale * windowSize.y / videoSize.y};
        model.size (size);
        if (!mShader)
          mShader = graphics.createTextureShader (videoFrame->mTextureType);
        mShader->use();

        // draw list
        for (auto& draw : mVideoFramesDraws) {
          draw.mVideoFrame->getTexture (graphics).setSource();
          cVec2 translate = {(windowSize.x / 2.f)  - ((videoSize.x / 2.f) * size.x) + draw.mOffset,
                             (windowSize.y / 2.f)  - ((videoSize.y / 2.f) * size.y)};
          model.setTranslate (translate);
          mShader->setModelProjection (model, orthoProjection);
          mQuad->draw();
          }

        videoRender.trimVideoBeforePts (playPts - (mHistory * videoFrame->mPtsDuration));
        }
      }
    }
  //}}}

  //{{{
  void drawTelly (cDvbStream& dvbStream, cGraphics& graphics) {

    cVec2 windowSize = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};

    // count numVideos
    int numVideos = 0;
    for (auto& pair : dvbStream.getServiceMap())
      if (pair.second.getStream (cDvbStream::eVideo).isEnabled())
        numVideos++;

    int curVideo = 0;
    float scale = (numVideos <= 1) ? 1.f : ((numVideos <= 4) ? 0.5f : 0.33f);
    for (auto& pair : dvbStream.getServiceMap()) {
      cDvbStream::cService& service = pair.second;
      if (service.getStream (cDvbStream::eVideo).isEnabled()) {
        curVideo++;
        cVideoRender& videoRender = dynamic_cast<cVideoRender&>(service.getStream (cDvbStream::eVideo).getRender());

        // get playerPts from stream
        int64_t playerPts = service.getStream (cDvbStream::eAudio).getPts();
        if (service.getStream (cDvbStream::eAudio).isEnabled()) {
          // update playerPts from audioPlayer
          cAudioRender& audioRender = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAudio).getRender());
          playerPts = audioRender.getPlayerPts();
          audioRender.setMute (curVideo != 1);
          if (curVideo <= 1)
            mFramesGraphic.draw (audioRender, videoRender, playerPts);
          }

        // draw telly pic
        cVideoFrame* videoFrame = videoRender.getVideoNearestFrameFromPts (playerPts);
        if (videoFrame) {
          cPoint videoSize = { videoRender.getWidth(), videoRender.getHeight() };
          if (!mQuad)
            mQuad = graphics.createQuad (videoSize);

          cTexture& texture = videoFrame->getTexture (graphics);
          if (!mShader)
            mShader = graphics.createTextureShader (texture.getTextureType());
          texture.setSource();
          mShader->use();

          cMat4x4 orthoProjection (0.f,static_cast<float>(windowSize.x), 0.f,static_cast<float>(windowSize.y),
                                   -1.f,1.f);
          cMat4x4 model;
          if (numVideos == 1) {
            //{{{  1 pic
            cVec2 size = {scale * windowSize.x / videoSize.x, scale * windowSize.y / videoSize.y};

            model.setTranslate ({(windowSize.x / 2.f)  - ((videoSize.x / 2.f) * size.x),
                                  (windowSize.y / 2.f)  - ((videoSize.y / 2.f) * size.y)});

            model.size (size);
            }
            //}}}
          else if (numVideos == 2) {
            //{{{  2 multiView
            cVec2 size = {scale * windowSize.x / videoSize.x, scale * windowSize.y / videoSize.y};

            if (curVideo == 1)
              model.setTranslate ({(windowSize.x / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y / 2.f)  - ((videoSize.y / 2.f) * size.y)});
            else
              model.setTranslate ({(windowSize.x * 3.f / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y / 2.f)  - ((videoSize.y / 2.f) * size.y)});
            model.size (size);
            }
            //}}}
          else if (numVideos == 3) {
            //{{{  3 multiView
            cVec2 size = {scale * windowSize.x / videoSize.x, scale * windowSize.y / videoSize.y};

            if (curVideo == 1)
              model.setTranslate ({(windowSize.x / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 3.f / 4.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 2)
              model.setTranslate ({(windowSize.x * 3.f / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 3.f / 4.f) - ((videoSize.y / 2.f) * size.y) });
            else
              model.setTranslate ({(windowSize.x / 2.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y / 4.f)  - ((videoSize.y / 2.f) * size.y)});
            model.size (size);
            }
            //}}}
          else if (numVideos == 4) {
            //{{{  4 multiView
            cVec2 size = {scale * windowSize.x / videoSize.x, scale * windowSize.y / videoSize.y};

            if (curVideo == 1)
              model.setTranslate ({(windowSize.x / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 3.f / 4.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 2)
              model.setTranslate ({(windowSize.x * 3.f / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 3.f / 4.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 3)
              model.setTranslate ({(windowSize.x / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y / 4.f)  - ((videoSize.y / 2.f) * size.y)});
            else
              model.setTranslate ({(windowSize.x * 3.f / 4.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y / 4.f)  - ((videoSize.y / 2.f) * size.y)});

            model.size (size);
            }
            //}}}
          else if (numVideos == 5) {
            //{{{  5 multiView
            cVec2 size = {scale * windowSize.x / videoSize.x, scale * windowSize.y / videoSize.y};

            if (curVideo == 1)
              model.setTranslate ({(windowSize.x * 1.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 5.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 2)
              model.setTranslate ({(windowSize.x * 5.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 5.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 3)
              model.setTranslate ({(windowSize.x * 1.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 1.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 4)
              model.setTranslate ({(windowSize.x * 5.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 1.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else
              model.setTranslate ({(windowSize.x / 2.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y / 2.f)  - ((videoSize.y / 2.f) * size.y)});

            model.size (size);
            }
            //}}}
          else if (numVideos == 6) {
            //{{{  6 multiView
            cVec2 size = {scale * windowSize.x / videoSize.x, scale * windowSize.y / videoSize.y};

            if (curVideo == 1)
              model.setTranslate ({(windowSize.x * 1.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 4.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 2)
              model.setTranslate ({(windowSize.x * 3.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 4.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 3)
              model.setTranslate ({(windowSize.x * 5.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 4.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 4)
              model.setTranslate ({(windowSize.x * 1.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 2.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 5)
              model.setTranslate ({(windowSize.x * 3.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 2.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else
              model.setTranslate ({(windowSize.x * 5.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 2.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});

            model.size (size);
            }
            //}}}
          else if (numVideos == 7) {
            //{{{  7 multiView
            cVec2 size = {scale * windowSize.x / videoSize.x, scale * windowSize.y / videoSize.y};

            if (curVideo == 1)
              model.setTranslate ({(windowSize.x * 1.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 5.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 2)
              model.setTranslate ({(windowSize.x * 3.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 5.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 3)
              model.setTranslate ({(windowSize.x * 5.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 5.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 4)
              model.setTranslate ({(windowSize.x * 1.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 1.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 5)
              model.setTranslate ({(windowSize.x * 3.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 1.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else if (curVideo == 6)
              model.setTranslate ({(windowSize.x * 5.f / 6.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y * 1.f / 6.f)  - ((videoSize.y / 2.f) * size.y)});
            else
              model.setTranslate ({(windowSize.x / 2.f)  - ((videoSize.x / 2.f) * size.x),
                                   (windowSize.y / 2.f)  - ((videoSize.y / 2.f) * size.y)});
            model.size (size);
            }
            //}}}
          mShader->setModelProjection (model, orthoProjection);
          mQuad->draw();

          //{{{  scaled multipic
          //cVec2 size = {mScale * windowSize.x / videoSize.x, mScale * windowSize.y / videoSize.y};
          //float replicate = floor (1.f / mScale);
          //for (float y = -videoSize.y * replicate; y <= videoSize.y * replicate; y += videoSize.y) {
            //for (float x = -videoSize.x * replicate; x <= videoSize.x * replicate; x += videoSize.x) {
              //// translate centre of video to centre of window
              //cVec2 translate = {(windowSize.x / 2.f)  - ((x + (videoSize.x / 2.f)) * size.x),
                                 //(windowSize.y / 2.f)  - ((y + (videoSize.y / 2.f)) * size.y)};

              //model.setTranslate (translate[curVideo]);
              //mShader->setModelProjection (model, orthoProjection);
              //mQuad->draw();
              //}
            //}
          //}}}
          videoRender.trimVideoBeforePts (playerPts - (mHistory * videoFrame->mPtsDuration));
          }
        }
      }
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
  void drawSubtitle (uint16_t sid, cRender& render, cGraphics& graphics) {
    (void)sid;

    cSubtitleRender& subtitleRender = dynamic_cast<cSubtitleRender&>(render);

    const float potSize = ImGui::GetTextLineHeight() / 2.f;
    size_t line = 0;
    while (line < subtitleRender.getNumLines()) {
      // draw subtitle line
      cSubtitleImage& image = subtitleRender.getImage (line);

      // draw clut color pots
      ImVec2 pos = ImGui::GetCursorScreenPos();
      for (size_t pot = 0; pot < image.mColorLut.max_size(); pot++) {
        ImVec2 potPos {pos.x + (pot % 8) * potSize, pos.y + (pot / 8) * potSize};
        uint32_t color = image.mColorLut[pot];
        ImGui::GetWindowDrawList()->AddRectFilled (
          potPos, {potPos.x + potSize - 1.f, potPos.y + potSize - 1.f}, color);
        }

      if (ImGui::InvisibleButton (fmt::format ("##subLog{}{}", sid, line).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        subtitleRender.toggleLog();

      // draw position
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{:3d},{:3d}", image.mX, image.mY).c_str());

      // create/update image texture
      if (!image.mTexture) // create
        image.mTexture = graphics.createTexture (cTexture::eRgba, {image.mWidth, image.mHeight});
      if (image.mDirty)
        image.mTexture->setPixels (&image.mPixels, nullptr);
      image.mDirty = false;

      // draw image, scaled to fit line
      ImGui::SameLine();
      ImGui::Image ((void*)(intptr_t)image.mTexture->getTextureId(),
                    {image.mWidth * ImGui::GetTextLineHeight()/image.mHeight, ImGui::GetTextLineHeight()});
      line++;
      }

    //{{{  add dummy lines to highwaterMark to stop jiggling
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
    //{{{  numpad codes
    // -------------------------------------------------------------------------------------
    // |    numlock       |        /           |        *             |        -            |
    // |GLFW_KEY_NUM_LOCK | GLFW_KEY_KP_DIVIDE | GLFW_KEY_KP_MULTIPLY | GLFW_KEY_KP_SUBTRACT|
    // |     0x11a        |      0x14b         |      0x14c           |      0x14d          |
    // |------------------------------------------------------------------------------------|
    // |        7         |        8           |        9             |         +           |
    // |  GLFW_KEY_KP_7   |   GLFW_KEY_KP_8    |   GLFW_KEY_KP_9      |  GLFW_KEY_KP_ADD;   |
    // |      0x147       |      0x148         |      0x149           |       0x14e         |
    // | -------------------------------------------------------------|                     |
    // |        4         |        5           |        6             |                     |
    // |  GLFW_KEY_KP_4   |   GLFW_KEY_KP_5    |   GLFW_KEY_KP_6      |                     |
    // |      0x144       |      0x145         |      0x146           |                     |
    // | -----------------------------------------------------------------------------------|
    // |        1         |        2           |        3             |       enter         |
    // |  GLFW_KEY_KP_1   |   GLFW_KEY_KP_2    |   GLFW_KEY_KP_3      |  GLFW_KEY_KP_ENTER  |
    // |      0x141       |      0x142         |      0x143           |       0x14f         |
    // | -------------------------------------------------------------|                     |
    // |        0                              |        .             |                     |
    // |  GLFW_KEY_KP_0                        | GLFW_KEY_KP_DECIMAL  |                     |
    // |      0x140                            |      0x14a           |                     |
    // --------------------------------------------------------------------------------------

    // glfw keycodes, they are platform specific
    // - ImGuiKeys small subset of normal keyboard keys
    // - have I misunderstood something here ?

    //constexpr int kNumpadNumlock = 0x11a;
    //constexpr int kNumpad0 = 0x140;
    //constexpr int kNumpad1 = 0x141;
    //constexpr int kNumpad2 = 0x142;
    //constexpr int kNumpad3 = 0x143;
    //constexpr int kNumpad4 = 0x144;
    //constexpr int kNumpad5 = 0x145;
    //constexpr int kNumpad6 = 0x146;
    //constexpr int kNumpad7 = 0x147;
    //constexpr int kNumpad8 = 0x148;
    //constexpr int kNumpad9 = 0x149;
    //constexpr int kNumpadDecimal = 0x14a;
    //constexpr int kNumpadDivide = 0x14b;
    //constexpr int kNumpadMultiply = 0x14c;
    //constexpr int kNumpadSubtract = 0x14d;
    //constexpr int kNumpadAdd = 0x14e;
    //constexpr int kNumpadEnter = 0x14f;
    //}}}

    ImGui::GetIO().WantTextInput = true;
    ImGui::GetIO().WantCaptureKeyboard = false;

    //bool altKeyPressed = ImGui::GetIO().KeyAlt;
    //bool ctrlKeyPressed = ImGui::GetIO().KeyCtrl;
    //bool shiftKeyPressed = ImGui::GetIO().KeyShift;

    for (int i = 0; i < ImGui::GetIO().InputQueueCharacters.Size; i++) {
      ImWchar ch = ImGui::GetIO().InputQueueCharacters[i];
      cLog::log (LOGINFO, fmt::format ("enter {:4x}", ch));
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

  std::array <size_t, 4> mPidMaxChars = { 3 };

  cQuad* mQuad = nullptr;
  cTextureShader* mShader = nullptr;

  float mScale = 1.f;
  float mOverlap = 4.f;
  int mHistory = 0;

  cFramesGraphic mFramesGraphic = { 2.f,1.f, 6.f,6.f };
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
    ImGui::Begin ("player", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

    mTellyView.draw ((cTellyApp&)app);

    ImGui::End();
    }
  //}}}

private:
  // vars
  cTellyView mTellyView;

  static cUI* create (const string& className) { return new cTellyUI (className); }
  inline static const bool mRegistered = registerClass ("telly", &create);
  };
//}}}

int main (int numArgs, char* args[]) {

  // params
  eLogLevel logLevel = LOGINFO;
  bool vsync = true;
  bool fullScreen = false;
  bool renderFirstService = false;
  cDvbMultiplex useMultiplex = kDvbMultiplexes[0];
  string filename;
  //{{{  parse command line args to params
  // parse params
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "log1") { logLevel = LOGINFO1; }
    else if (param == "log2") { logLevel = LOGINFO2; }
    else if (param == "log3") { logLevel = LOGINFO3; }
    else if (param == "full") { fullScreen = true; }
    else if (param == "free") { vsync = false; }
    else if (param == "first") { renderFirstService = true; }
    else {
      // assume param is filename unless it matches multiplex name
      filename = param;
      for (auto& multiplex : kDvbMultiplexes)
        if (param == multiplex.mName) {
          useMultiplex = multiplex;
          filename = "";
          break;
          }
      }
    }
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("telly"));
  if (filename.empty())
    cLog::log (LOGINFO, fmt::format ("using multiplex {}", useMultiplex.mName));

  // list static registered classes
  cUI::listRegisteredClasses();

  // app
  cTellyApp tellyApp ({1920/2, 1080/2}, fullScreen, vsync);
  tellyApp.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
  tellyApp.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 18.f));

  tellyApp.setSource (filename, kRecordRoot, useMultiplex, !filename.empty() || renderFirstService);

  tellyApp.mainUILoop();

  return EXIT_SUCCESS;
  }

// cTvUI.cpp
//{{{  includes
#include <cstdint>
#include <array>
#include <vector>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"
#include "../implot/implot.h"

// ui
#include "../ui/cUI.h"

#include "../platform/cPlatform.h"
#include "../graphics/cGraphics.h"
#include "../tv/cTvApp.h"

// dvb
#include "../dvb/cVideoDecoder.h"
#include "../dvb/cAudioDecoder.h"
#include "../dvb/cSubtitleDecoder.h"
#include "../dvb/cDvbTransportStream.h"

// utils
#include "../utils/cLog.h"
#include "../utils/utils.h"

using namespace std;
//}}}

class cTellyView {
public:
  //{{{
  void draw (cTvApp& app) {

    if (app.getPlatform().hasVsync()) {
      //{{{  vsync button
      if (toggleButton ("vSync", app.getPlatform().getVsync()))
          app.getPlatform().toggleVsync();

      // fps text
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      ImGui::SameLine();
      }
      //}}}
    if (app.getPlatform().hasFullScreen()) {
      //{{{  fullScreen button
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      ImGui::SameLine();
      }
      //}}}
    mMainTabIndex = interlockedButtons ({"services", "pids", "recorded"}, mMainTabIndex, {0.f,0.f}, true);
    //{{{  vertice debug
    ImGui::SameLine();
    ImGui::TextUnformatted (fmt::format ("{}:{}",
                            ImGui::GetIO().MetricsRenderVertices,
                            ImGui::GetIO().MetricsRenderIndices/3).c_str());
    //}}}

    cDvbTransportStream* dvbTransportStream = app.getDvbTransportStream();
    if (dvbTransportStream) {
      if (dvbTransportStream->hasTdtTime()) {
        //{{{  draw tdtTime
        ImGui::SameLine();
        ImGui::TextUnformatted (dvbTransportStream->getTdtTimeString().c_str());
        }
        //}}}
      //{{{  draw packet,errors
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{}:{} ",
                              dvbTransportStream->getNumPackets(), dvbTransportStream->getNumErrors()).c_str());
      //}}}
      if (dvbTransportStream->hasDvbSource()) {
        //{{{  draw dvbSource signal,errors
        ImGui::SameLine();
        ImGui::TextUnformatted (dvbTransportStream->getSignalString().c_str());

        ImGui::SameLine();
        ImGui::TextUnformatted (dvbTransportStream->getErrorString().c_str());
        }
        //}}}

      // draw scrollable contents
      ImGui::PushFont (app.getMonoFont());
      ImGui::BeginChild ("##telly", {0.f,0.f}, false,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);

      switch (mMainTabIndex) {
        //{{{
        case 0: { // services
          drawServices (*dvbTransportStream, app.getGraphics());
          break;
          }
        //}}}
        //{{{
        case 1: { // pids
          drawPids (*dvbTransportStream, app.getGraphics());
          break;
          }
        //}}}
        //{{{
        case 2: { // recorded
          drawRecorded (*dvbTransportStream);
          break;
          }
        //}}}
        }

      ImGui::EndChild();
      ImGui::PopFont();
      }

    }
  //}}}
private:
  //{{{
  void drawServices (cDvbTransportStream& dvbTransportStream, cGraphics& graphics) {
  // draw service map

    mPlotIndex = 0;

    for (auto& serviceItem : dvbTransportStream.getServiceMap()) {
      cDvbTransportStream::cService& service =  serviceItem.second;
      //{{{  calc max field sizes, may jiggle for a couple of draws
      while (service.getChannelName().size() > mMaxNameSize)
        mMaxNameSize = service.getChannelName().size();

      while (service.getSid() > pow (10, mMaxSidSize))
        mMaxSidSize++;

      while (service.getProgramPid() > pow (10, mMaxPgmSize))
        mMaxPgmSize++;

      while (service.getVidPid() > pow (10, mMaxVidSize))
        mMaxVidSize++;

      while (service.getAudPid() > pow (10, mMaxAudSize))
        mMaxAudSize++;

      while (service.getSubPid() > pow (10, mMaxSubSize))
        mMaxSubSize++;
      //}}}

      // draw channel name pgm,sid
      if (ImGui::Button (fmt::format ("{:{}s} {:{}d}:{:{}d}",
                         service.getChannelName(), mMaxNameSize,
                         service.getProgramPid(), mMaxPgmSize,
                         service.getSid(), mMaxSidSize).c_str())) {
        //{{{  toggle all
        service.toggleVideoDecode();
        service.toggleAudioDecode();
        service.toggleSubtitleDecode();
        }
        //}}}
      if (service.getVidPid()) {
        //{{{  got vid
        ImGui::SameLine();
        if (toggleButton (fmt::format ("vid:{:{}d}:{}",
                                       service.getVidPid(), mMaxVidSize, service.getVidStreamTypeName()).c_str(),
                          service.getVideoDecoder()))
          service.toggleVideoDecode();
        }
        //}}}
      if (service.getAudPid()) {
        //{{{  got aud
        ImGui::SameLine();
        if (toggleButton (fmt::format ("aud:{:{}d}:{}",
                                       service.getAudPid(), mMaxAudSize, service.getAudStreamTypeName()).c_str(),
                          service.getAudioDecoder()))
          service.toggleAudioDecode();
        }
        //}}}
      if (service.getAudOtherPid()) {
        //{{{  got aud
        ImGui::SameLine();
        if (toggleButton (fmt::format ("{:{}d}:{}",
                                       service.getAudOtherPid(), mMaxAudSize, service.getAudOtherStreamTypeName()).c_str(),
                          service.getAudioOtherDecoder()))
          service.toggleAudioOtherDecode();
        }
        //}}}
      if (service.getSubPid()) {
        //{{{  got sub
        ImGui::SameLine();
        if (toggleButton (fmt::format ("{}:{:{}d}",
                                       service.getSubStreamTypeName(), service.getSubPid(), mMaxSubSize).c_str(),
                          service.getSubtitleDecoder()))
          service.toggleSubtitleDecode();
        }
        //}}}
      if (service.getChannelRecord()) {
        //{{{  got record
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("rec:{}", service.getChannelRecordName()).c_str());
        }
        //}}}

      int64_t playPts = 0;
      if (service.getAudioDecoder())
        playPts = service.getAudioDecoder()->getPlayPts();

      int64_t lastPts = 0;
      if (service.getAudioDecoder())
        lastPts = drawAudio (*service.getAudioDecoder(), lastPts, graphics);
      if (service.getAudioOtherDecoder())
        lastPts = drawAudio (*service.getAudioOtherDecoder(), lastPts, graphics);
      if (service.getVideoDecoder())
        lastPts = drawVideo (*service.getVideoDecoder(), lastPts, playPts, graphics);
      if (service.getSubtitleDecoder())
        lastPts = drawSubtitle (*service.getSubtitleDecoder(), lastPts, graphics);
      }
    }
  //}}}
  //{{{
  void drawPids (cDvbTransportStream& dvbTransportStream, cGraphics& graphics) {
  // draw pidInfoMap

    // calc error number width
    int errorDigits = 1;
    while (dvbTransportStream.getNumErrors() > pow (10, errorDigits))
      errorDigits++;

    int prevSid = 0;
    for (auto& pidInfoItem : dvbTransportStream.getPidInfoMap()) {
      // iterate for pidInfo
      cDvbTransportStream::cPidInfo& pidInfo = pidInfoItem.second;

      // draw separator, crude test for new service, fails sometimes
      if ((pidInfo.mSid != prevSid) && (pidInfo.mStreamType != 5) && (pidInfo.mStreamType != 11))
        ImGui::Separator();

      // draw pid label
      ImVec2 cursorPos = ImGui::GetCursorPos();
      ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {}",
                              pidInfo.mPackets, mPacketDigits, pidInfo.mErrors, errorDigits, pidInfo.mPid,
                              getFullPtsString (pidInfo.mPts), pidInfo.getTypeName()).c_str());

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
      if ((pidInfo.mStreamType == 0) && (pidInfo.mSid != 0xFFFF))
        streamText = fmt::format ("{} ", pidInfo.mSid) + streamText;
      ImGui::TextUnformatted (streamText.c_str());

      cDvbTransportStream::cService* service = dvbTransportStream.getService (pidInfo.mSid);
      if (service) {
        ImGui::SetCursorPos (cursorPos);
        if (ImGui::InvisibleButton (fmt::format ("##pid{}", pidInfo.mPid).c_str(),
                                    {ImGui::GetWindowWidth(), ImGui::GetTextLineHeight()}))
          if (pidInfo.mPid == service->getSubPid())
            service->toggleSubtitleDecode();

        if (pidInfo.mPid == service->getSubPid()) {
          cSubtitleDecoder* subtitleDecoder = service->getSubtitleDecoder();
          if (subtitleDecoder)
            drawSubtitle (*subtitleDecoder, 0, graphics);
          }
        }

      // adjust packet number width
      if (pidInfo.mPackets > pow (10, mPacketDigits))
        mPacketDigits++;

      prevSid = pidInfo.mSid;
      }
    }
  //}}}
  //{{{
  void drawRecorded (cDvbTransportStream& dvbTransportStream) {
  // list recorded items

    for (auto& program : dvbTransportStream.getRecordPrograms())
      ImGui::TextUnformatted (program.c_str());
    }
  //}}}

  //{{{
  int64_t drawVideo (cVideoDecoder& video, int64_t pts, int64_t playPts, cGraphics& graphics) {

    int64_t lastPts = plotValues (pts, video, 0xffffffff);

    uint32_t* buf = video.getFrame (playPts);
    if (buf) {
      if (mTexture == nullptr) // create
        mTexture = graphics.createTexture ({video.getWidth(), video.getHeight()}, (uint8_t*)buf);
      else
        mTexture->setPixels ((uint8_t*)buf);

      ImGui::Image ((void*)(intptr_t)mTexture->getTextureId(), {video.getWidth()/4.f,video.getHeight()/4.f});
      }

    drawMiniLog (video.getLog());
    return lastPts;
    }
  //}}}
  //{{{
  int64_t drawAudio (cAudioDecoder& audio, int64_t pts, cGraphics& graphics) {

    (void)graphics;
    int64_t lastPts = plotValues (pts, audio, 0xff00ffff);
    drawMiniLog (audio.getLog());
    return lastPts;
    }
  //}}}
  //{{{
  int64_t drawSubtitle (cSubtitleDecoder& subtitle, int64_t pts, cGraphics& graphics) {

    int64_t lastPts = plotValues (pts, subtitle, 0xff00ff00);

    const float potSize = ImGui::GetTextLineHeight() / 2.f;
    size_t line = 0;
    while (line < subtitle.getNumLines()) {
      // draw subtitle line
      cSubtitleImage& image = subtitle.getImage (line);

      // draw clut color pots
      ImVec2 pos = ImGui::GetCursorScreenPos();
      for (size_t pot = 0; pot < image.mColorLut.max_size(); pot++) {
        ImVec2 potPos {pos.x + (pot % 8) * potSize, pos.y + (pot / 8) * potSize};
        uint32_t color = image.mColorLut[pot];
        ImGui::GetWindowDrawList()->AddRectFilled (
          potPos, {potPos.x + potSize - 1.f, potPos.y + potSize - 1.f}, color);
        }
      if (ImGui::InvisibleButton (fmt::format ("##pot{}", line).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        subtitle.toggleLog();

      // draw position
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{:3d},{:3d}", image.mX, image.mY).c_str());

      // create/update image texture
      if (image.mTexture == nullptr) // create
        image.mTexture = graphics.createTexture ({image.mWidth, image.mHeight}, image.mPixels);
      else if (image.mDirty) // update
        image.mTexture->setPixels (image.mPixels);
      image.mDirty = false;

      // draw image, scaled to fit line
      ImGui::SameLine();
      ImGui::Image ((void*)(intptr_t)image.mTexture->getTextureId(),
                    {image.mWidth * ImGui::GetTextLineHeight()/image.mHeight, ImGui::GetTextLineHeight()});
      line++;
      }

    //{{{  add dummy lines to highwaterMark to stop jiggling
    while (line < subtitle.getHighWatermark()) {
      if (ImGui::InvisibleButton (fmt::format ("##line{}", line).c_str(),
                                  {ImGui::GetWindowWidth() - ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        subtitle.toggleLog();
      line++;
      }
    //}}}

    drawMiniLog (subtitle.getLog());
    return lastPts;
    }
  //}}}
  //{{{
  int64_t plotValues (int64_t lastPts, cDecoder& decoder, uint32_t color) {

    (void)color;

    if (!lastPts)
      lastPts = decoder.getLastPts();

    decoder.setRefPts (lastPts);
    decoder.setMapSize (static_cast<size_t>(25 + ImGui::GetWindowWidth()));

    auto lamda = [](void* data, int idx) {
      int64_t pts = 0;
      return ImPlotPoint (-idx, ((cDecoder*)data)->getOffsetValue (idx * (90000/25), pts));
      };

    if (ImPlot::BeginPlot (fmt::format ("##plot{}", mPlotIndex++).c_str(), NULL, NULL,
                           {ImGui::GetWindowWidth(), 4*ImGui::GetTextLineHeight()},
                           ImPlotFlags_NoLegend,
                           ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit,
                           ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit)) {
      ImPlot::PlotStairsG ("line", lamda, &decoder, (int)ImGui::GetWindowWidth());
      //ImPlot::PlotBarsG ("line", lamda, &decoder, (int)ImGui::GetWindowWidth(), 2.0);
      //ImPlot::PlotLineG ("line", lamda, &decoder, (int)ImGui::GetWindowWidth());
      //ImPlot::PlotScatterG ("line", lamda, &decoder, (int)ImGui::GetWindowWidth());
      ImPlot::EndPlot();
      }

    return lastPts;
    }
  //}}}
  //{{{  vars
  uint8_t mMainTabIndex = 0;

  int mPacketDigits = 3;
  int mMaxPidPackets = 3;

  size_t mMaxNameSize = 3;
  size_t mMaxSidSize = 3;
  size_t mMaxPgmSize = 3;
  size_t mMaxVidSize = 3;
  size_t mMaxAudSize = 3;
  size_t mMaxSubSize = 3;

  int mPlotIndex = 0;
  //}}}
  cTexture* mTexture = nullptr;
  };

// cTvUI
class cTvUI : public cUI {
public:
  cTvUI (const string& name) : cUI(name) {}
  virtual ~cTvUI() = default;

  //{{{
  void addToDrawList (cApp& app) final {
  // draw into window

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    ImGui::Begin ("player", &mOpen, ImGuiWindowFlags_NoTitleBar);

    mTellyView.draw ((cTvApp&)app);

    ImGui::End();
    }
  //}}}

private:
  // vars
  bool mOpen = true;

  cTellyView mTellyView;

  static cUI* create (const string& className) { return new cTvUI (className); }
  inline static const bool mRegistered = registerClass ("tv", &create);
  };

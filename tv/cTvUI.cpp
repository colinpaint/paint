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
#include "../dvb/cVideoRender.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cDvbStream.h"

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
    mMainTabIndex = interlockedButtons ({"tv","services", "pids", "recorded"}, mMainTabIndex, {0.f,0.f}, true);

    if (app.getDvbStream()) {
      cDvbStream& dvbStream = *app.getDvbStream();
      if (dvbStream.hasTdtTime()) {
        //{{{  draw tdtTime
        ImGui::SameLine();
        ImGui::TextUnformatted (dvbStream.getTdtTimeString().c_str());
        }
        //}}}
      //{{{  draw dvbStream packet,errors
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{}:{}", dvbStream.getNumPackets(), dvbStream.getNumErrors()).c_str());
      //}}}
      if (dvbStream.hasDvbSource()) {
        //{{{  draw dvbStream::dvbSource signal,errors
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("{}:{}", dvbStream.getSignalString(), dvbStream.getErrorString()).c_str());
        }
        //}}}
      //{{{  vertex,index debug
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{}:{}",
                              ImGui::GetIO().MetricsRenderVertices,
                              ImGui::GetIO().MetricsRenderIndices/3).c_str());
      //}}}

      // draw tab in childWindow
      ImGui::PushFont (app.getMonoFont());
      ImGui::BeginChild ("telly", {0.f,0.f}, false,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);

      switch (mMainTabIndex) {
        case 0: drawTv (dvbStream, app.getGraphics()); break;
        case 1: drawServices (dvbStream, app.getGraphics()); break;
        case 2: drawPids (dvbStream, app.getGraphics()); break;
        case 3: drawRecorded (dvbStream, app.getGraphics()); break;
        }

      ImGui::EndChild();
      ImGui::PopFont();
      }
    }
  //}}}
private:
  //{{{
  void drawTv (cDvbStream& dvbStream, cGraphics& graphics) {

    for (auto& pair : dvbStream.getServiceMap()) {
      cDvbStream::cService& service = pair.second;

      if (service.getStream (cDvbStream::eAud).isEnabled()) {
        cAudioRender& audio = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAud).getRender());
        int64_t playPts = audio.getPlayPts();

        if (service.getStream (cDvbStream::eVid).isEnabled()) {
          cVideoRender& video = dynamic_cast<cVideoRender&>(service.getStream (cDvbStream::eVid).getRender());
          cTexture* texture = video.getTexture (playPts, graphics);
          if (texture)
            ImGui::Image ((void*)(intptr_t)texture->getTextureId(),
                          {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()});
                          //{(float)video.getWidth(),(float)video.getHeight()});

          ImGui::SetCursorPos ({0.f,0.f});
          ImGui::TextUnformatted (fmt::format ("pts:{} a:{} v:{}",
                                    getPtsString (playPts), audio.getInfoString(), video.getInfoString()).c_str());
          break;
          }
        }

      while (service.getChannelName().size() > mMaxNameChars)
        mMaxNameChars = service.getChannelName().size();
      }

    // overlay channel buttons
    for (auto& pair : dvbStream.getServiceMap())
      if (ImGui::Button (fmt::format ("{:{}s}", pair.second.getChannelName(), mMaxNameChars).c_str()))
        pair.second.toggleAll();
    }
  //}}}
  //{{{
  void drawServices (cDvbStream& dvbStream, cGraphics& graphics) {
  // draw service map

    mPlotIndex = 0;
    for (auto& pair : dvbStream.getServiceMap()) {
      cDvbStream::cService& service = pair.second;
      //{{{  calc max field sizes, may jiggle for a couple of draws
      while (service.getChannelName().size() > mMaxNameChars)
        mMaxNameChars = service.getChannelName().size();
      while (service.getSid() > pow (10, mMaxSidChars))
        mMaxSidChars++;
      while (service.getProgramPid() > pow (10, mMaxPgmChars))
        mMaxPgmChars++;

      for (size_t streamType = cDvbStream::eVid; streamType < cDvbStream::eLast; streamType++) {
        uint16_t pid = service.getStream (streamType).getPid();
        while (pid > pow (10, mMaxChars[streamType]))
          mMaxChars[streamType]++;
        }
      //}}}

      //{{{  draw channel name pgm,sid
      if (ImGui::Button (fmt::format ("{:{}s} {:{}d}:{:{}d}",
                         service.getChannelName(), mMaxNameChars,
                         service.getProgramPid(), mMaxPgmChars,
                         service.getSid(), mMaxSidChars).c_str()))
        service.toggleAll();
      //}}}
      for (size_t streamType = cDvbStream::eVid; streamType < cDvbStream::eLast; streamType++) {
        // iterate service streams
        cDvbStream::cStream& stream = service.getStream (streamType);
        if (stream.isDefined()) {
          ImGui::SameLine();
          if (toggleButton (fmt::format ("{}{:{}d}:{}##{}",
                                         stream.getLabel(),
                                         stream.getPid(), mMaxChars[streamType],
                                         stream.getTypeName(), streamType).c_str(), stream.isEnabled()))
            service.toggleStream (streamType);
          }
        }

      if (service.getChannelRecord()) {
        //{{{  draw record
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("rec:{}", service.getChannelRecordName()).c_str());
        }
        //}}}

      // audio provides playPts
      int64_t playPts = 0;
      if (service.getStream (cDvbStream::eAud).isEnabled())
        playPts = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAud).getRender()).getPlayPts();

      // render enabled streams
      cDvbStream::cStream& stream = service.getStream (cDvbStream::eVid);
      if (stream.isEnabled())
        drawVideo (dynamic_cast<cVideoRender&>(stream.getRender()), graphics, playPts);

      stream = service.getStream (cDvbStream::eAud);
      if (stream.isEnabled())
        drawAudio (dynamic_cast<cAudioRender&>(stream.getRender()), graphics);

      stream = service.getStream (cDvbStream::eAudOther);
      if (stream.isEnabled())
        drawAudio (dynamic_cast<cAudioRender&>(stream.getRender()), graphics);

      stream = service.getStream (cDvbStream::eSub);
      if (stream.isEnabled())
        drawSubtitle (dynamic_cast<cSubtitleRender&>(stream.getRender()), graphics);
      }
    }
  //}}}
  //{{{
  void drawPids (cDvbStream& dvbStream, cGraphics& graphics) {
  // draw pidInfoMap

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
      if ((pidInfo.mSid != prevSid) && (pidInfo.mStreamType != 5) && (pidInfo.mStreamType != 11))
        ImGui::Separator();

      // draw pid label
      ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {}",
                              pidInfo.mPackets, mPacketChars, pidInfo.mErrors, errorChars, pidInfo.mPid,
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

      // adjust packet number width
      if (pidInfo.mPackets > pow (10, mPacketChars))
        mPacketChars++;

      prevSid = pidInfo.mSid;
      }
    }
  //}}}
  //{{{
  void drawRecorded (cDvbStream& dvbStream, cGraphics& graphics) {
  // list recorded items

    (void)graphics;
    for (auto& program : dvbStream.getRecordPrograms())
      ImGui::TextUnformatted (program.c_str());
    }
  //}}}

  //{{{
  void plotValues (cRender& render, uint32_t color) {

    (void)color;

    int64_t lastPts = render.getLastPts();

    render.setRefPts (lastPts);
    render.setMapSize (static_cast<size_t>(25 + ImGui::GetWindowWidth()));

    auto lamda = [](void* data, int idx) {
      int64_t pts = 0;
      return ImPlotPoint (-idx, ((cRender*)data)->getOffsetValue (idx * (90000/25), pts));
      };

    if (ImPlot::BeginPlot (fmt::format ("##plot{}", mPlotIndex++).c_str(), NULL, NULL,
                           {ImGui::GetWindowWidth(), 4*ImGui::GetTextLineHeight()},
                           ImPlotFlags_NoLegend,
                           ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit,
                           ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit)) {
      ImPlot::PlotStairsG ("line", lamda, &render, (int)ImGui::GetWindowWidth());
      //ImPlot::PlotBarsG ("line", lamda, &render, (int)ImGui::GetWindowWidth(), 2.0);
      //ImPlot::PlotLineG ("line", lamda, &render, (int)ImGui::GetWindowWidth());
      //ImPlot::PlotScatterG ("line", lamda, &render, (int)ImGui::GetWindowWidth());
      ImPlot::EndPlot();
      }
    }
  //}}}
  //{{{
  void drawVideo (cVideoRender& video, cGraphics& graphics, int64_t playPts) {

    plotValues (video, 0xffffffff);

    ImGui::TextUnformatted (video.getInfoString().c_str());

    cTexture* texture = video.getTexture (playPts, graphics);
    if (texture)
      ImGui::Image ((void*)(intptr_t)texture->getTextureId(), {video.getWidth()/4.f,video.getHeight()/4.f});

    drawMiniLog (video.getLog());
    }
  //}}}
  //{{{
  void drawAudio (cAudioRender& audio, cGraphics& graphics) {

    (void)graphics;

    plotValues (audio, 0xff00ffff);
    ImGui::TextUnformatted (audio.getInfoString().c_str());

    drawMiniLog (audio.getLog());
    }
  //}}}
  //{{{
  void drawSubtitle (cSubtitleRender& subtitle, cGraphics& graphics) {

    plotValues (subtitle, 0xff00ff00);

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
    }
  //}}}
  //{{{  vars
  uint8_t mMainTabIndex = 0;

  int64_t mMaxPidPackets = 0;
  size_t mPacketChars = 3;

  size_t mMaxNameChars = 3;
  size_t mMaxSidChars = 3;
  size_t mMaxPgmChars = 3;

  std::array <size_t, 4> mMaxChars = { 3 };

  int mPlotIndex = 0;
  //}}}
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

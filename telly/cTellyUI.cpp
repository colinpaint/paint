// cTellyUI.cpp
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
#include "../telly/cTellyApp.h"

// dvb
#include "../dvb/cVideoRender.h"
#include "../dvb/cVideoFrame.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cAudioFrame.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cDvbStream.h"

// utils
#include "../utils/cLog.h"
#include "../utils/utils.h"

using namespace std;
//}}}

//{{{
class cRenderIcon {
public:
  cRenderIcon (float videoLines, float audioLines, float pixelsPerVideoFrame, float pixelsPerChannel)
    : mVideoLines(videoLines), mAudioLines(audioLines),
      mPixelsPerVideoFrame(pixelsPerVideoFrame), mPixelsPerChannel(pixelsPerChannel) {}

  //{{{
  void draw (cAudioRender& audio, cVideoRender& video, int64_t playPts) {

    ImVec2 pos = {ImGui::GetCursorScreenPos().x + ImGui::GetWindowWidth(),
                  ImGui::GetCursorScreenPos().y + ImGui::GetWindowHeight() - (0.5f * ImGui::GetTextLineHeight())};

    drawPower (audio, playPts, pos);

    pos.x -= mMaxOffset;

    // draw playPts centre bar
    ImGui::GetWindowDrawList()->AddRectFilled (
      {pos.x, pos.y - mVideoLines * (3.f/4.f) * ImGui::GetTextLineHeight()},
      {pos.x + 1.f, pos.y + mAudioLines * (3.f/4.f) * ImGui::GetTextLineHeight()},
      0xffc0c0c0);

    float ptsScale = mPixelsPerVideoFrame / video.getPtsDuration();

    { // lock video during iterate
    shared_lock<shared_mutex> lock (video.getSharedMutex());
    for (auto& frame : video.getFrames()) {
      //{{{  draw video
      cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame.second);
      float offset1 = (videoFrame->mPts - playPts) * ptsScale;
      float offset2 = offset1 + (videoFrame->mPtsDuration * ptsScale) - 1.f;
      addOffset (offset1, mMaxOffset, mMaxDisplayOffset);

      // pesSize I white/ P yellow/ B green
      ImGui::GetWindowDrawList()->AddRectFilled (
        {pos.x + offset1, pos.y},
        {pos.x + offset2, pos.y - addValue ((float)videoFrame->mPesSize, mMaxPesSize, mMaxDisplayPesSize, mVideoLines)},
        (videoFrame->mFrameType == 'I') ? 0xffffffff : (videoFrame->mFrameType == 'P') ? 0xff00ffff : 0xff00ff00);

      // grey decodeTime
      ImGui::GetWindowDrawList()->AddRectFilled (
        {pos.x + offset1, pos.y},
        {pos.x + offset2, pos.y - addValue ((float)videoFrame->mTimes[0], mMaxDecodeTime, mMaxDisplayDecodeTime, mVideoLines)},
        0x60ffffff);

      // magenta queueSize in trailing gap
      ImGui::GetWindowDrawList()->AddRectFilled (
        {pos.x + offset1, pos.y},
        {pos.x + offset1 + 1.f, pos.y - addValue ((float)videoFrame->mQueueSize, mMaxQueueSize, mMaxDisplayQueueSize, mVideoLines - 1.f)},
        0xffff00ff);
      }
      //}}}
    for (auto frame : video.getFreeFrames()) {
      //{{{  draw free video
      cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
      float offset1 = (videoFrame->mPts - playPts) * ptsScale;
      float offset2 = offset1 + (videoFrame->mPtsDuration * ptsScale) - 1.f;

      ImGui::GetWindowDrawList()->AddRectFilled (
        {pos.x + offset1, pos.y},
        {pos.x + offset2, pos.y - addValue ((float)videoFrame->mPesSize, mMaxPesSize, mMaxDisplayPesSize, mVideoLines)},
        0xe0808080);
      }
      //}}}
    }

    { // lock audio during iterate
    shared_lock<shared_mutex> lock (audio.getSharedMutex());
    for (auto& frame : audio.getFrames()) {
      //{{{  draw audio
      cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame.second);
      float offset1 = (audioFrame->mPts - playPts) * ptsScale;
      float offset2 = offset1 + (audioFrame->mPtsDuration * ptsScale) - 1.f;
      addOffset (offset1, mMaxOffset, mMaxDisplayOffset);

      ImGui::GetWindowDrawList()->AddRectFilled (
        {pos.x + offset1, pos.y + 1.f},
        {pos.x + offset2, pos.y + 1.f + addValue (audioFrame->mPeakValues[0], mMaxPower, mMaxDisplayPower, mAudioLines)},
        0xff00ffff);
      }
      //}}}
    for (auto frame : audio.getFreeFrames()) {
      //{{{  draw free audio
      cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);
      float offset1 = (audioFrame->mPts - playPts) * ptsScale;
      float offset2 = offset1 + (audioFrame->mPtsDuration * ptsScale) - 1.f;

      ImGui::GetWindowDrawList()->AddRectFilled (
        {pos.x + offset1, pos.y + 1.f},
        {pos.x + offset2, pos.y + 1.f + addValue (audioFrame->mPeakValues[0], mMaxPower, mMaxDisplayPower, mAudioLines)},
        0xC0808080);
      }
      //}}}
    }

    // slowly track scaling back to max display values from max values
    agc (mMaxPower, mMaxDisplayPower, 250.f, 0.5f);
    agc (mMaxPesSize, mMaxDisplayPesSize, 250.f, 10000.f);
    agc (mMaxDecodeTime, mMaxDisplayDecodeTime, 250.f, 1000.f);
    agc (mMaxQueueSize, mMaxDisplayQueueSize, 250.f, 4.f);
    agc (mMaxOffset, mMaxDisplayOffset, 250.f, ImGui::GetTextLineHeight());
    }
  //}}}

private:
  //{{{
  void drawPower (cAudioRender& audio, int64_t playPts, ImVec2 pos) {

    cAudioFrame* audioFrame = audio.getAudioFramePts (playPts);
    if (audioFrame) {
      pos.y += 1.f;
      float height = 3.f * ImGui::GetTextLineHeight();

      if (audioFrame->mNumChannels == 6) {
        // 5.1
        float x = pos.x - (5 * mPixelsPerChannel);

        // draw channels reordered
        for (size_t i = 0; i < 5; i++) {
          ImGui::GetWindowDrawList()->AddRectFilled (
            {x, pos.y},
            {x + mPixelsPerChannel - 1.f, pos.y + (audioFrame->mPowerValues[kChannelOrder[i]] * height)},
            0xff00ff00);
          x += mPixelsPerChannel;
          }

        // draw woofer as red
        ImGui::GetWindowDrawList()->AddRectFilled (
          {pos.x - (5 * mPixelsPerChannel), pos.y},
          {pos.x - 1.f, pos.y + (audioFrame->mPowerValues[kChannelOrder[5]] * height)},
          0x800000ff);
        }

      else  {
        // other - draw channels left to right
        float width = mPixelsPerChannel * audioFrame->mNumChannels;
        for (size_t i = 0; i < audioFrame->mNumChannels; i++)
          ImGui::GetWindowDrawList()->AddRectFilled (
            {pos.x - width + (i * mPixelsPerChannel), pos.y},
            {pos.x - width + ((i+1) * mPixelsPerChannel) - 1.f, pos.y + (audioFrame->mPowerValues[i] * height)},
            0xff00ff00);
        }
      }
    }
  //}}}

  //{{{
  void addOffset (float offset, float& maxOffset, float& maxDisplayOffset) {

    if (offset > maxOffset)
      maxOffset = offset;

    if (offset > maxDisplayOffset)
      maxDisplayOffset = offset;
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
  const float mPixelsPerChannel;

  float mMaxOffset = 0.f;
  float mMaxDisplayOffset = 0.f;

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

class cTellyView {
public:
  //{{{
  void draw (cTellyApp& app) {

    cGraphics& graphics = app.getGraphics();

    // clear bgnd
    graphics.background (cPoint((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

    // draw tabs
    ImGui::SameLine();
    mTab = (eTab)interlockedButtons (kTabNames, (uint8_t)mTab, {0.f,0.f}, true);

    // draw fullScreen
    if (app.getPlatform().hasFullScreen()) {
      //{{{  fullScreen button
      ImGui::SameLine();
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      }
      //}}}

    // scale
    ImGui::SameLine();
    ImGui::SetNextItemWidth (4.f * ImGui::GetTextLineHeight());
    ImGui::DragFloat ("##scale", &mScale, 0.01f, 0.05f, 16.f, "scale%3.2f");

    // map size
    ImGui::SameLine();
    ImGui::SetNextItemWidth (3.f * ImGui::GetTextLineHeight());
    ImGui::DragInt ("##aud", &mAudioFrameMapSize, 0.25f, 2, 100, "aud %d");
    if (ImGui::IsItemHovered())
      mAudioFrameMapSize = max (2, min (100, mAudioFrameMapSize + static_cast<int>(ImGui::GetIO().MouseWheel)));

    // draw frameRate
    ImGui::SameLine();
    ImGui::TextUnformatted (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
    ImGui::SameLine();
    if (app.getPlatform().hasVsync() && app.getPlatform().getVsync())
      ImGui::TextUnformatted ("vsync");
    //app.getPlatform().toggleVsync();

    ImGui::SameLine();
    if (app.getDvbStream()) {
      // draw dvbStream info
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

      //{{{  draw vertex,index info
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{}:{}",
                              ImGui::GetIO().MetricsRenderVertices,
                              ImGui::GetIO().MetricsRenderIndices/3).c_str());
      //}}}

      // draw tab childWindow, monospaced font
      ImGui::PushFont (app.getMonoFont());
      ImGui::BeginChild ("tab", {0.f,0.f}, false,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);
      switch (mTab) {
        case eTelly:    drawTelly    (dvbStream, graphics, app.getDecoderOptions()); break;
        case eServices: drawServices (dvbStream, graphics, app.getDecoderOptions()); break;
        case ePids:     drawPidMap   (dvbStream, graphics, app.getDecoderOptions()); break;
        case eRecorded: drawRecorded (dvbStream, graphics, app.getDecoderOptions()); break;
        case eHistory:  drawHistory  (dvbStream, graphics, app.getDecoderOptions()); break;
        }

      ImGui::EndChild();
      ImGui::PopFont();
      }
    }
  //}}}
private:
  enum eTab { eTelly, eHistory, eServices, ePids, eRecorded };
  inline static const vector<string> kTabNames = { "telly", "history", "services", "pids", "recorded" };

  //{{{
  void drawTelly (cDvbStream& dvbStream, cGraphics& graphics, uint16_t decoderOptions) {

    (void)graphics;
    cVec2 windowSize = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};

    for (auto& pair : dvbStream.getServiceMap()) {
      //{{{  draw telly
      cDvbStream::cService& service = pair.second;
      if (!service.getStream (cDvbStream::eVid).isEnabled())
        continue;

      cVideoRender& video = dynamic_cast<cVideoRender&>(service.getStream (cDvbStream::eVid).getRender());

      int64_t playPts = service.getStream (cDvbStream::eAud).getPts();
      if (service.getStream (cDvbStream::eAud).isEnabled()) {
        cAudioRender& audio = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAud).getRender());
        audio.setFrameMapSize (mAudioFrameMapSize);
        playPts = audio.getPlayPts();
        mRenderIcon.draw (audio, video, playPts);
        }

      cVideoFrame* videoFrame = video.getVideoFramePts (playPts);
      if (videoFrame) {
        cPoint videoSize = {video.getWidth(), video.getHeight()};
        if (!mQuad)
          mQuad = graphics.createQuad (videoSize);

        cTexture& texture = videoFrame->getTexture (graphics);
        if (!mShader)
          mShader = graphics.createTextureShader (texture.getTextureType());
        texture.setSource();
        mShader->use();

        cMat4x4 orthoProjection (0.f,static_cast<float>(windowSize.x), 0.f,static_cast<float>(windowSize.y), -1.f,1.f);
        cVec2 size = {mScale * windowSize.x / videoSize.x, mScale * windowSize.y / videoSize.y};

        cMat4x4 model;
        model.size (size);

        int i = 0;
        float replicate = floor (1.f / mScale);
        for (float y = -videoSize.y * replicate; y <= videoSize.y * replicate; y += videoSize.y) {
          for (float x = -videoSize.x * replicate; x <= videoSize.x * replicate; x += videoSize.x) {
            // translate centre of video to centre of window
            cVec2 translate = {(windowSize.x / 2.f)  - ((x + (videoSize.x / 2.f)) * size.x),
                               (windowSize.y / 2.f)  - ((y + (videoSize.y / 2.f)) * size.y)};

            model.setTranslate (translate);
            mShader->setModelProjection (model, orthoProjection);
            mQuad->draw();
            i++;
            }
          }
        float xoff = (windowSize.x / 2.f) - ((videoSize.x / 2.f) * size.x);
        cLog::log (LOGINFO, fmt::format ("nnn {} {} = {} - {}",
                                         i, xoff, windowSize.x / 2.f, (videoSize.x / 2.f) * size.x));

        video.trimVideoBeforePts (playPts);
        }
      }
      //}}}

    for (auto& pair : dvbStream.getServiceMap()) {
      //{{{  draw services/channels
      cDvbStream::cService& service = pair.second;
      if (ImGui::Button (fmt::format ("{:{}s}", service.getChannelName(), mMaxNameChars).c_str()))
        service.toggleAll (decoderOptions);

      if (service.getStream (cDvbStream::eAud).isDefined()) {
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("{}{}",
          service.getStream (cDvbStream::eAud).isEnabled() ? "*":"", service.getNowTitleString()).c_str());
        //ImGui::TextUnformatted (fmt::format ("{} {}", audio.getInfo(), video.getInfo()).c_str());
        }

      while (service.getChannelName().size() > mMaxNameChars)
        mMaxNameChars = service.getChannelName().size();
      }
      //}}}
    }
  //}}}
  //{{{
  void drawHistory (cDvbStream& dvbStream, cGraphics& graphics, uint16_t decoderOptions) {

    (void)decoderOptions;
    cVec2 windowSize = {ImGui::GetWindowWidth(), ImGui::GetWindowHeight()};

    for (auto& pair : dvbStream.getServiceMap()) {
      cDvbStream::cService& service = pair.second;
      if (!service.getStream (cDvbStream::eVid).isEnabled())
        continue;

      cVideoRender& video = dynamic_cast<cVideoRender&>(service.getStream (cDvbStream::eVid).getRender());

      int64_t playPts = service.getStream (cDvbStream::eAud).getPts();
      if (service.getStream (cDvbStream::eAud).isEnabled()) {
        cAudioRender& audio = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAud).getRender());
        audio.setFrameMapSize (mAudioFrameMapSize);
        playPts = audio.getPlayPts();
        mRenderIcon.draw (audio, video, playPts);
        }

      cPoint videoSize = {video.getWidth(), video.getHeight()};
      if (!mQuad)
        mQuad = graphics.createQuad (videoSize);

      cMat4x4 model;
      cMat4x4 orthoProjection (0.f,static_cast<float>(windowSize.x) , 0.f, static_cast<float>(windowSize.y), -1.f, 1.f);

      cVec2 size = {mScale * windowSize.x / videoSize.x, mScale * windowSize.y / videoSize.y};
      model.size (size);

      cVideoFrame* videoFrame = video.getVideoFramePts (playPts);
      if (videoFrame) {
        if (!mShader)
          mShader = graphics.createTextureShader (videoFrame->mTextureType);
        mShader->use();

        int64_t pts = playPts;
        videoFrame = video.getVideoFramePts (pts);
        if (videoFrame) {
          videoFrame->getTexture (graphics).setSource();
          cVec2 translate = {(windowSize.x / 2.f)  - ((videoSize.x / 2.f) * size.x),
                             (windowSize.y / 2.f)  - ((videoSize.y / 2.f) * size.y)};
          model.setTranslate (translate);
          mShader->setModelProjection (model, orthoProjection);
          mQuad->draw();
          }

        pts -= video.getPtsDuration();
        video.trimVideoBeforePts (pts);
        }
      }
    }
  //}}}
  //{{{
  void drawServices (cDvbStream& dvbStream, cGraphics& graphics, uint16_t decoderOptions) {

    mPlotIndex = 0;
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

      for (size_t streamType = cDvbStream::eVid; streamType < cDvbStream::eLast; streamType++) {
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
        service.toggleAll (decoderOptions);

      for (size_t streamType = cDvbStream::eVid; streamType < cDvbStream::eLast; streamType++) {
       // iterate definedStreams
        cDvbStream::cStream& stream = service.getStream (streamType);
        if (stream.isDefined()) {
          ImGui::SameLine();
          // draw definedStream button - sid ensuresd unique button name
          if (toggleButton (fmt::format ("{}{:{}d}:{}##{}",
                                         stream.getLabel(),
                                         stream.getPid(), mPidMaxChars[streamType], stream.getTypeName(),
                                         service.getSid()).c_str(), stream.isEnabled()))
           dvbStream.toggleStream (service, streamType, decoderOptions);
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
      if (service.getStream (cDvbStream::eAud).isEnabled())
        playPts = dynamic_cast<cAudioRender&>(service.getStream (cDvbStream::eAud).getRender()).getPlayPts();

      for (size_t streamType = cDvbStream::eVid; streamType < cDvbStream::eLast; streamType++) {
        // iterate enabledStreams, drawing plots,logs,images
        cDvbStream::cStream& stream = service.getStream (streamType);
        if (stream.isEnabled()) {
          switch (streamType) {
            case cDvbStream::eVid: drawVideo (service.getSid(), stream.getRender(), graphics, playPts); break;
            case cDvbStream::eAud:
            case cDvbStream::eAds: drawAudio (service.getSid(), stream.getRender(), graphics);  break;
            case cDvbStream::eSub: drawSubtitle (service.getSid(), stream.getRender(), graphics);  break;
            default:;
            }
          }
        }
      }
    }
  //}}}
  //{{{
  void drawPidMap (cDvbStream& dvbStream, cGraphics& graphics, uint16_t decoderOptions) {
  // draw pidInfoMap

    (void)graphics;
    (void)decoderOptions;

    // calc error number width
    int errorChars = 1;
    while (dvbStream.getNumErrors() > pow (10, errorChars))
      errorChars++;

    int prevSid = 0;
    for (auto& pidInfoItem : dvbStream.getPidInfoMap()) {
      // iterate for pidInfo
      cDvbStream::cPidInfo& pidInfo = pidInfoItem.second;

      // draw separator, crude test for new service, fails sometimes
      if ((pidInfo.getSid() != prevSid) && (pidInfo.getStreamTypeId() != 5) && (pidInfo.getStreamTypeId() != 11))
        ImGui::Separator();

      // draw pid label
      ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {} {}",
                              pidInfo.mPackets, mPacketChars, pidInfo.mErrors, errorChars, pidInfo.getPid(),
                              getFullPtsString (pidInfo.getPts()),
                              getFullPtsString (pidInfo.getDts()),
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
      if ((pidInfo.getStreamTypeId() == 0) && (pidInfo.getSid() != 0xFFFF))
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
  void drawRecorded (cDvbStream& dvbStream, cGraphics& graphics, uint16_t decoderOptions) {
  // list recorded items

    (void)graphics;
    (void)decoderOptions;

    for (auto& program : dvbStream.getRecordPrograms())
      ImGui::TextUnformatted (program.c_str());
    }
  //}}}

  //{{{
  void plotValues (uint16_t sid, cRender& render, uint32_t color) {

    (void)color;
    int64_t lastPts = render.getLastPts();

    render.setRefPts (lastPts);
    render.setMapSize (static_cast<size_t>(25 + ImGui::GetWindowWidth()));

    auto lambda = [](void* data, int idx) {
      int64_t pts = 0;
      return ImPlotPoint (-idx, ((cRender*)data)->getOffsetValue (idx * (90000/25), pts));
      };

    // draw plot - sid ensures unique name
    if (ImPlot::BeginPlot (fmt::format ("##plot{}", sid).c_str(),
                           {ImGui::GetWindowWidth(), 4*ImGui::GetTextLineHeight()},
                           ImPlotFlags_NoLegend)) {
      ImPlot::SetupAxis (ImAxis_X1, "", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit);
      ImPlot::SetupAxis (ImAxis_Y1, "", ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_AutoFit);
      //ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1000);
      //ImPlot::SetupAxisFormat(ImAxis_Y1, "$%.0f");
      //ImPlot::SetupLegend(ImPlotLocation_North);
      ImPlot::SetupFinish();

      ImPlot::PlotStairsG ("line", lambda, &render, (int)ImGui::GetWindowWidth());
      //ImPlot::PlotBarsG ("line", lambda, &render, (int)ImGui::GetWindowWidth(), 2.0);
      //ImPlot::PlotLineG ("line", lambda, &render, (int)ImGui::GetWindowWidth());
      //ImPlot::PlotScatterG ("line", lambda, &render, (int)ImGui::GetWindowWidth());
      ImPlot::EndPlot();
      }
    }
  //}}}
  //{{{
  void drawVideo (uint16_t sid, cRender& render, cGraphics& graphics, int64_t playPts) {

    (void)graphics;
    (void)playPts;

    cVideoRender& video = dynamic_cast<cVideoRender&>(render);
    plotValues (sid, video, 0xffffffff);
    ImGui::TextUnformatted (video.getInfo().c_str());
    drawMiniLog (video.getLog());
    }
  //}}}
  //{{{
  void drawAudio (uint16_t sid, cRender& render, cGraphics& graphics) {

    (void)graphics;
    cAudioRender& audio = dynamic_cast<cAudioRender&>(render);

    plotValues (sid, audio, 0xff00ffff);
    ImGui::TextUnformatted (audio.getInfo().c_str());

    drawMiniLog (audio.getLog());
    }
  //}}}
  //{{{
  void drawSubtitle (uint16_t sid, cRender& render, cGraphics& graphics) {

    cSubtitleRender& subtitle = dynamic_cast<cSubtitleRender&>(render);
    plotValues (sid, subtitle, 0xff00ff00);

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
      if (!image.mTexture) // create
        image.mTexture = graphics.createTexture (cTexture::eRgba, {image.mWidth, image.mHeight});
      image.mTexture->setPixels (&image.mPixels);
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
  eTab mTab = eTelly;

  int64_t mMaxPidPackets = 0;
  size_t mPacketChars = 3;

  size_t mMaxNameChars = 3;
  size_t mMaxSidChars = 3;
  size_t mMaxPgmChars = 3;

  std::array <size_t, 4> mPidMaxChars = {3};

  int mPlotIndex = 0;

  cQuad* mQuad = nullptr;
  cTextureShader* mShader = nullptr;

  int mAudioFrameMapSize = 6;
  float mScale = 1.f;

  cRenderIcon mRenderIcon = {2.f,1.f, 4.f,6.f};
  //}}}
  };

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
    ImGui::Begin ("player", nullptr,
                  ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
                  //ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                  //ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse |


    mTellyView.draw ((cTellyApp&)app);

    ImGui::End();
    }
  //}}}

private:
  // vars
  bool mOpen = true;
  cTellyView mTellyView;
  static cUI* create (const string& className) { return new cTellyUI (className); }
  inline static const bool mRegistered = registerClass ("telly", &create);
  };

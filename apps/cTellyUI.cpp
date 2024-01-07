// tellyUI.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
#endif

#include <cstdint>
#include <string>
#include <array>

#include "../date/include/date/date.h"
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../app/myImgui.h"

#include "cTellyUI.h"

using namespace std;
//}}}
namespace {
  constexpr size_t kMaxSubtitleLines = 4;
  const vector <string> kTabNames = { "telly", "pids", "recorded", "epg" };
  }

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
                       ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

    // draw select rect
    bool hover = ImGui::IsMouseHoveringRect (mTL, mBR);
    if ((hover || (mSelect != eUnselected)) && (mSelect != eSelectedFull))
      ImGui::GetWindowDrawList()->AddRect (mTL, mBR, hover ? 0xff20ffff : 0xff20ff20, 4.f, 0, 4.f);

    bool enabled = mService.getStream (cTransportStream::eVideo).isEnabled();
    if (enabled) {
      //{{{  get audio playPts
      int64_t playPts = mService.getStream (cTransportStream::eAudio).getRender().getPts();
      if (mService.getStream (cTransportStream::eAudio).isEnabled()) {
        // get playPts from audioStream
        cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (cTransportStream::eAudio).getRender());
        if (audioRender.getPlayer())
          playPts = audioRender.getPlayer()->getPts();
        }
      //}}}
      if (!selectFull || (mSelect != eUnselected)) {
        cVideoRender& videoRender = dynamic_cast<cVideoRender&>(mService.getStream (cTransportStream::eVideo).getRender());
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
              dynamic_cast<cSubtitleRender&> (mService.getStream (cTransportStream::eSubtitle).getRender());

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
        if (mService.getStream (cTransportStream::eAudio).isEnabled()) {
          //{{{  audio mute, audioMeter, framesGraphic
          cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (cTransportStream::eAudio).getRender());

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

    //{{{  draw title
    std::string title = mService.getName();
    if (!enabled || (mSelect == eSelectedFull))
      title += " " + mService.getNowTitleString();

    // drop shadow title at top
    ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
    ImGui::SetCursorPos (pos);

    ImGui::PushFont (tellyApp.getLargeFont());
    ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
    ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
    ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
    ImGui::PopFont();

    pos.y += ImGui::GetTextLineHeight() * 1.5f;
    //}}}
    if (tellyApp.getOptions()->mShowEpg) {
      //{{{  draw epg in childWindow, hit epg, select action
      ImGui::BeginChild (fmt::format ("epg##{}", mService.getSid()).c_str(),
                         mSize - ImVec2 ({0.f, ImGui::GetTextLineHeight() * 1.5f}),
                         ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);

      ImVec2 posEpg = {0.f,0.f};
      for (auto& epgItem : mService.getTodayEpg()) {
        auto time = epgItem.first;
        if (enabled && (mSelect != eSelectedFull))
          time += epgItem.second->getDuration();
        if (time > transportStream.getNowTdt()) {
          // draw epgItem title
          std::string epgTitle = date::format ("%H:%M ", date::floor<chrono::seconds>(epgItem.first)) +
                                   epgItem.second->getTitle();

          // drop shadow epg
          ImGui::SetCursorPos (posEpg);
          ImGui::TextColored ({0.f,0.f,0.f,1.f}, epgTitle.c_str());
          ImGui::SetCursorPos (posEpg - ImVec2(2.f,2.f));
          ImGui::TextColored ({1.f, 1.f,1.f,1.f}, epgTitle.c_str());

          posEpg.y += ImGui::GetTextLineHeight();
          }
        }

      ImGui::SetCursorPos ({0.f,0.f});
      if (ImGui::InvisibleButton (fmt::format ("epgInvisible##{}", mService.getSid()).c_str(), mSize)) {
        // hit view, select action
        if (!mService.getStream (cTransportStream::eVideo).isEnabled())
          mService.enableStreams();
        else if (mSelect == eUnselected)
          mSelect = eSelected;
        else if (mSelect == eSelected)
          mSelect = eSelectedFull;
        else if (mSelect == eSelectedFull)
          mSelect = eSelected;

        result = true;
        }

      ImGui::EndChild();
      }
      //}}}
    else if (ImGui::InvisibleButton (fmt::format ("viewInvisible##{}", mService.getSid()).c_str(), mSize)) {
      //{{{  hit view, select action
      if (!mService.getStream (cTransportStream::eVideo).isEnabled())
        mService.enableStreams();
      else if (mSelect == eUnselected)
        mSelect = eSelected;
      else if (mSelect == eSelected)
        mSelect = eSelectedFull;
      else if (mSelect == eSelectedFull)
        mSelect = eSelected;

      result = true;
      }
      //}}}

    if (mSelect == eSelectedFull) {
      //{{{  draw ptsFromStart
      std::string ptsFromStartString = utils::getPtsString (mService.getPtsFromStart());

      ImGui::PushFont (tellyApp.getLargeFont());

      pos = ImVec2 (mSize.x - ImGui::GetTextLineHeight() * 10.f, mSize.y - ImGui::GetTextLineHeight()*1.5f);
      ImGui::SetCursorPos (pos);
      ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsFromStartString.c_str());
      ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
      ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsFromStartString.c_str());

      ImGui::PopFont();
      }
      //}}}

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
        if (service.getStream (cTransportStream::eVideo).isDefined())
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

// public
cTellyUI::cTellyUI() : mMultiView (*(new cMultiView())) {}
//{{{
void cTellyUI::draw (cApp& app) {

  ImGui::SetKeyboardFocusHere();

  app.getGraphics().clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                            (int32_t)ImGui::GetIO().DisplaySize.y});

  ImGui::SetNextWindowPos ({0.f,0.f});
  ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
  ImGui::Begin ("telly", nullptr, ImGuiWindowFlags_NoTitleBar |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                  ImGuiWindowFlags_NoBackground);

  cTellyApp& tellyApp = (cTellyApp&)app;
  if (tellyApp.hasTransportStream()) {
    cTransportStream& transportStream = tellyApp.getTransportStream();

    // multiView piccies
    mMultiView.draw (tellyApp, transportStream);

    // menu
    ImGui::SetCursorPos ({0.f, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeight() * 1.5f});
    ImGui::BeginChild ("menu", {0.f, ImGui::GetTextLineHeight() * 1.5f});

    ImGui::SetCursorPos ({0.f,0.f});
    //{{{  tab interlocked buttons
    mTab = (eTab)interlockedButtons (kTabNames, (uint8_t)mTab, {0.f,0.f}, true);
    tellyApp.setShowEpg (mTab == eEpg);
    //}}}
    //{{{  subtitle button
    ImGui::SameLine();
    if (toggleButton ("sub", tellyApp.getOptions()->mShowSubtitle))
      tellyApp.toggleShowSubtitle();
    //}}}
    if (tellyApp.getOptions()->mHasMotionVectors) {
      //{{{  motionVectors button
      ImGui::SameLine();
      if (toggleButton ("motion", tellyApp.getOptions()->mShowMotionVectors))
        tellyApp.toggleShowMotionVectors();
      }
      //}}}
    if (tellyApp.getPlatform().hasFullScreen()) {
      //{{{  fullScreen button
      ImGui::SameLine();
      if (toggleButton ("full", tellyApp.getPlatform().getFullScreen()))
        tellyApp.getPlatform().toggleFullScreen();
      }
      //}}}
    if (tellyApp.getPlatform().hasVsync()) {
      //{{{  vsync button
      ImGui::SameLine();
      if (toggleButton ("vsync", tellyApp.getPlatform().getVsync()))
        tellyApp.getPlatform().toggleVsync();
      }
      //}}}
    //{{{  frameRate info
    ImGui::SameLine();
    ImGui::TextUnformatted (fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
    //}}}
    if (tellyApp.hasDvbSource()) {
      //{{{  dvbSource signal,errors
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{} {}", tellyApp.getDvbSource().getTuneString(),
                                                    tellyApp.getDvbSource().getStatusString()).c_str());
      }
      //}}}
    if (transportStream.getNumErrors()) {
      //{{{  transportStream errors
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("error:{}", tellyApp.getTransportStream().getNumErrors()).c_str());
      }
      //}}}
    ImGui::EndChild();

    //{{{  tabbed subMenu
    switch (mTab) {
      case ePids:
        ImGui::PushFont (tellyApp.getMonoFont());
        ImGui::SetCursorPos ({0.f,0.f});
        drawPids (transportStream);
        ImGui::PopFont();
        break;

      case eRecord:
        ImGui::PushFont (tellyApp.getMonoFont());
        ImGui::SetCursorPos ({0.f,0.f});
        drawRecordedFileNames (transportStream);
        ImGui::PopFont();
        break;

      default:;
      }
    //}}}
    if (transportStream.hasFirstTdt()) {
      //{{{  clock
      //ImGui::TextUnformatted (transportStream.getTdtString().c_str());
      ImGui::SetCursorPos ({ ImGui::GetWindowWidth() - 90.f, 0.f} );
      clockButton ("clock", transportStream.getNowTdt(), { 80.f, 80.f });
      }
      //}}}
    }

  keyboard (tellyApp);
  ImGui::End();
  }
//}}}

// private
//{{{
void cTellyUI::drawPids (cTransportStream& transportStream) {
// draw pids

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
//{{{
void cTellyUI::drawRecordedFileNames (cTransportStream& transportStream) {

  for (auto& program : transportStream.getRecordedFileNames())
    ImGui::TextUnformatted (program.c_str());
  }
//}}}

//{{{
void cTellyUI::keyboard (cTellyApp& tellyApp) {

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
    { false, false,  false, ImGuiKey_Space,      [this,&tellyApp]{ hitSpace (tellyApp); }},
    { false, true,   false, ImGuiKey_LeftArrow,  [this,&tellyApp]{ hitControlLeft (tellyApp); }},
    { false, true,   false, ImGuiKey_RightArrow, [this,&tellyApp]{ hitControlRight (tellyApp); }},
    { false, false,  false, ImGuiKey_LeftArrow,  [this,&tellyApp]{ hitLeft (tellyApp); }},
    { false, false,  false, ImGuiKey_RightArrow, [this,&tellyApp]{ hitRight (tellyApp); }},
    { false, false,  false, ImGuiKey_UpArrow,    [this,&tellyApp]{ hitUp (tellyApp); }},
    { false, false,  false, ImGuiKey_DownArrow,  [this,&tellyApp]{ hitDown (tellyApp); }},
    { false, false,  false, ImGuiKey_Enter,      [this,&tellyApp]{ mMultiView.hitEnter(); }},
    { false, false,  false, ImGuiKey_F,          [this,&tellyApp]{ tellyApp.getPlatform().toggleFullScreen(); }},
    { false, false,  false, ImGuiKey_S,          [this,&tellyApp]{ tellyApp.toggleShowSubtitle(); }},
    { false, false,  false, ImGuiKey_L,          [this,&tellyApp]{ tellyApp.toggleShowMotionVectors(); }},
    { false, false,  false, ImGuiKey_T,          [this,&tellyApp]{ hitTab (eTelly); }},
    { false, false,  false, ImGuiKey_P,          [this,&tellyApp]{ hitTab (ePids); }},
    { false, false,  false, ImGuiKey_R,          [this,&tellyApp]{ hitTab (eRecord); }},
    { false, false,  false, ImGuiKey_E,          [this,&tellyApp]{ hitTab (eEpg); }},
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
void cTellyUI::hitSpace (cTellyApp& tellyApp) {

  if (tellyApp.hasFileSource())
    tellyApp.togglePlay();
  else
    mMultiView.hitSpace();
  }
//}}}
//{{{
void cTellyUI::hitControlLeft (cTellyApp& tellyApp) {

  if (tellyApp.hasFileSource())
    tellyApp.skip (-90000 / 25);
  }
//}}}
//{{{
void cTellyUI::hitControlRight (cTellyApp& tellyApp) {

  if (tellyApp.hasFileSource())
    tellyApp.skip (90000 / 25);
  }
//}}}
//{{{
void cTellyUI::hitLeft (cTellyApp& tellyApp) {

  if (tellyApp.hasFileSource())
    tellyApp.skip (-90000);
  else
    mMultiView.hitLeft();
  }
//}}}
//{{{
void cTellyUI::hitRight (cTellyApp& tellyApp) {

  if (tellyApp.hasFileSource())
    tellyApp.skip (90000 / 25);
  else
    mMultiView.hitRight();
  }
//}}}
//{{{
void cTellyUI::hitUp (cTellyApp& tellyApp) {

  if (tellyApp.hasFileSource())
    tellyApp.skip (-900000 / 25);
  else
    mMultiView.hitUp();
  }
//}}}
//{{{
void cTellyUI::hitDown (cTellyApp& tellyApp) {

  if (tellyApp.hasFileSource())
    tellyApp.skip (900000 / 25);
  else
    mMultiView.hitDown();
  }
//}}}
//{{{
void cTellyUI::hitTab (eTab tab) {
  mTab = (tab == mTab) ? eTelly : tab;
  }
//}}}

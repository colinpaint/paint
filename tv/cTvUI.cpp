// cTvUI.cpp
//{{{  includes
#include <cstdint>
#include <array>
#include <vector>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

// ui
#include "../ui/cUI.h"

#include "../platform/cPlatform.h"
#include "../graphics/cGraphics.h"
#include "../tv/cTvApp.h"

// dvb
#include "../dvb/cDvbSubtitle.h"
#include "../dvb/cDvbTransportStream.h"

// utils
#include "../utils/cLog.h"
#include "../utils/utils.h"

using namespace std;
//}}}

// cTvView
class cTvView {
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

    //{{{  fullScreen button
    if (app.getPlatform().hasFullScreen()) {
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      ImGui::SameLine();
      }
    //}}}
    //{{{  vertice debug
    ImGui::TextUnformatted (fmt::format ("{}:{}",
                            ImGui::GetIO().MetricsRenderVertices,
                            ImGui::GetIO().MetricsRenderIndices/3).c_str());
    //}}}

    cDvbTransportStream* dvbTransportStream = app.getDvbTransportStream();
    if (dvbTransportStream) {
      ImGui::SameLine();
      if (toggleButton ("sub", dvbTransportStream->getDecodeSubtitle()))
        dvbTransportStream->toggleDecodeSubtitle();

      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{} ", dvbTransportStream->getNumPackets()).c_str());

      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format("{} ", dvbTransportStream->getNumErrors()).c_str());

      ImGui::SameLine();
      ImGui::TextUnformatted (dvbTransportStream->getSignalString().c_str());

      ImGui::SameLine();
      ImGui::TextUnformatted (dvbTransportStream->getErrorString().c_str());

      // draw scrollable contents
      ImGui::PushFont (app.getMonoFont());
      ImGui::BeginChild ("##tv", {0.f,0.f}, false,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);

      drawContents (dvbTransportStream, app.getGraphics());

      ImGui::EndChild();
      ImGui::PopFont();
      }

    }
  //}}}
private:
  //{{{
  void drawContents (cDvbTransportStream* dvbTransportStream, cGraphics& graphics) {
  // simple enough to use ImGui interface directly

    // list recorded items
    for (auto& program : dvbTransportStream->getRecordPrograms())
      ImGui::TextUnformatted (program.c_str());

    // calc error number width
    int errorDigits = 1;
    while (dvbTransportStream->getNumErrors() > pow (10, errorDigits))
      errorDigits++;

    int prevSid = 0;
    for (auto& pidInfoItem : dvbTransportStream->getPidInfoMap()) {
      // iterate for pidInfo
      cPidInfo& pidInfo = pidInfoItem.second;

      // draw separator, crude test for new service, fails sometimes
      if ((pidInfo.mSid != prevSid) && (pidInfo.mStreamType != 5) && (pidInfo.mStreamType != 11))
        ImGui::Separator();

      // draw pid label
      ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {}",
                              pidInfo.mPackets, mPacketDigits, pidInfo.mErrors, errorDigits, pidInfo.mPid,
                              getFullPtsString (pidInfo.mPts), pidInfo.getTypeString()).c_str());

      // get pos for stream info
      ImGui::SameLine();
      ImVec2 pos = ImGui::GetCursorScreenPos();

      // draw stream bar
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

      if ((pidInfo.mStreamType == 6) && (dvbTransportStream->hasSubtitle (pidInfo.mSid)))
        drawSubtitle (dvbTransportStream->getSubtitle (pidInfo.mSid), graphics);

      // adjust packet number width
      if (pidInfo.mPackets > pow (10, mPacketDigits))
        mPacketDigits++;

      prevSid = pidInfo.mSid;
      }
    }
  //}}}
  //{{{
  void drawSubtitle (cDvbSubtitle& subtitle, cGraphics& graphics) {

    float clutPotSize = ImGui::GetTextLineHeight()/2.f;

    // highWaterMark numLines
    if (subtitle.mRects.size() > subtitle.mMaxLines)
      subtitle.mMaxLines = subtitle.mRects.size();

    size_t line = 0;
    for (; line < subtitle.mRects.size(); line++) {
      // line order is reverse y order
      size_t lineIndex = subtitle.mRects.size() - 1 - line;

      // draw clut color pots
      ImVec2 pos = ImGui::GetCursorScreenPos();
      for (int pot = 0; pot < subtitle.mRects[lineIndex].mClutSize; pot++) {
        ImVec2 clutPotPos {pos.x + (pot % 8) * clutPotSize, pos.y + (pot / 8) * clutPotSize};
        uint32_t color = subtitle.mRects[lineIndex].mClut[pot]; // possible swizzle
        ImGui::GetWindowDrawList()->AddRectFilled (
          clutPotPos, {clutPotPos.x + clutPotSize - 1.f, clutPotPos.y + clutPotSize - 1.f}, color);
        }
      ImGui::InvisibleButton (fmt::format ("##pot{}", line).c_str(),
                              {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()});

      // draw position
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{},{:3d}",
                              subtitle.mRects[lineIndex].mX, subtitle.mRects[lineIndex].mY).c_str());

      // subtitle image
      if (subtitle.mTextures[lineIndex] == nullptr) // create
        subtitle.mTextures[lineIndex] = graphics.createTexture (
          {subtitle.mRects[lineIndex].mWidth, subtitle.mRects[lineIndex].mHeight},
          (uint8_t*)subtitle.mRects[lineIndex].mPixData);
      else if (subtitle.mChanged) // update
        subtitle.mTextures[lineIndex]->setPixels ((uint8_t*)subtitle.mRects[lineIndex].mPixData);

      // draw image, scaled to fit
      ImGui::SameLine();
      float scale = ImGui::GetTextLineHeight() / subtitle.mRects[lineIndex].mHeight;
      ImGui::Image ((void*)(intptr_t)subtitle.mTextures[lineIndex]->getTextureId(),
                    {subtitle.mRects[lineIndex].mWidth * scale, ImGui::GetTextLineHeight()});
      }

    // pad out to maxLines, stops jumping about
    for (; line < subtitle.mMaxLines; line++)
      ImGui::InvisibleButton (fmt::format ("##empty{}", line).c_str(),
                              {ImGui::GetWindowWidth() - ImGui::GetTextLineHeight(),ImGui::GetTextLineHeight()});

    // reset changed flag
    subtitle.mChanged = false;
    }
  //}}}

  int mPacketDigits = 0;
  int mMaxPidPackets = 0;
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

    mTvView.draw ((cTvApp&)app);

    ImGui::End();
    }
  //}}}

private:
  // vars
  bool mOpen = true;
  cTvView mTvView;

  static cUI* create (const string& className) { return new cTvUI (className); }
  inline static const bool mRegistered = registerClass ("tv", &create);
  };

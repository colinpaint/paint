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

    float potSize = ImGui::GetTextLineHeight() / 2.f;

    size_t line = 0;
    while (line < subtitle.getNumRegions()) {
      // line order is reverse y order
      cSubtitleImage& image = *subtitle.mImages[subtitle.mImages.size() - 1 - line];

      // draw clut color pots
      ImVec2 pos = ImGui::GetCursorScreenPos();
      for (size_t pot = 0; pot < image.mColorLut.max_size(); pot++) {
        ImVec2 potPos {pos.x + (pot % 8) * potSize, pos.y + (pot / 8) * potSize};
        uint32_t color = image.mColorLut[pot];
        ImGui::GetWindowDrawList()->AddRectFilled (potPos,
                                                   {potPos.x + potSize - 1.f, potPos.y + potSize - 1.f}, color);
        }
      ImGui::InvisibleButton (fmt::format ("##pot{}", line).c_str(),
                              {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()});

      // draw position
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{},{:3d}", image.mX, image.mY).c_str());

      // create/update image texture
      if (image.mTexture == nullptr) // create
        image.mTexture = graphics.createTexture ({image.mWidth, image.mHeight}, image.mPixels);
      else if (image.mPixelsChanged) // update
        image.mTexture->setPixels (image.mPixels);
      image.mPixelsChanged = false;

      // draw image, scaled to fit
      ImGui::SameLine();
      float scale = ImGui::GetTextLineHeight() / image.mHeight;
      ImGui::Image ((void*)(intptr_t)image.mTexture->getTextureId(),
                    { image.mWidth * scale, ImGui::GetTextLineHeight()});
      line++;
      }

    // pad lines to highwater mark, stops jumping about
    while (line < subtitle.mImages.size()) {
      ImGui::InvisibleButton (fmt::format ("##empty{}", line).c_str(),
                              {ImGui::GetWindowWidth() - ImGui::GetTextLineHeight(),ImGui::GetTextLineHeight()});
      line++;
      }
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

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

// utils
#include "../utils/cLog.h"
#include "../utils/date.h"
#include "../utils/utils.h"

// dvb
#include "../dvb/cDvbTransportStream.h"
#include "../dvb/cSubtitle.h"

#include "../tv/cTvApp.h"

using namespace std;
//}}}
//{{{  channels const
const vector<string> kRadio1 = {"r1", "a128"};
const vector<string> kRadio2 = {"r2", "a128"};
const vector<string> kRadio3 = {"r3", "a320"};
const vector<string> kRadio4 = {"r4", "a64"};
const vector<string> kRadio5 = {"r5", "a128"};
const vector<string> kRadio6 = {"r6", "a128"};

const vector<string> kBbc1   = {"bbc1", "a128"};
const vector<string> kBbc2   = {"bbc2", "a128"};
const vector<string> kBbc4   = {"bbc4", "a128"};
const vector<string> kNews   = {"news", "a128"};
const vector<string> kBbcSw  = {"sw", "a128"};

const vector<string> kWqxr  = {"http://stream.wqxr.org/js-stream.aac"};
const vector<string> kDvb  = {"dvb"};

const vector<string> kRtp1  = {"rtp 1"};
const vector<string> kRtp2  = {"rtp 2"};
const vector<string> kRtp3  = {"rtp 3"};
const vector<string> kRtp4  = {"rtp 4"};
const vector<string> kRtp5  = {"rtp 5"};
//}}}

namespace {
  int gPacketDigits = 0;
  int gMaxPidPackets = 0;
  //{{{
  void drawTransportStream (cDvbTransportStream* dvbTransportStream) {

    // list recorded items
    for (auto& recordItem : dvbTransportStream->getRecordItems())
      ImGui::TextUnformatted (recordItem.c_str());

    // width of error field
    int errorDigits = 1;
    while (dvbTransportStream->getNumErrors() > pow (10, errorDigits))
      errorDigits++;

    int prevSid = 0;
    for (auto& pidInfoItem : dvbTransportStream->getPidInfoMap()) {
      // iterate for pidInfo
      cPidInfo& pidInfo = pidInfoItem.second;

      // draw separator
      if ((pidInfo.mSid != prevSid) && (pidInfo.mStreamType != 5) && (pidInfo.mStreamType != 11))
        ImGui::Separator();

      // draw pic text
      ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {}",
                              pidInfo.mPackets, gPacketDigits, pidInfo.mErrors, errorDigits, pidInfo.mPid,
                              getFullPtsString (pidInfo.mPts), pidInfo.getTypeString()).c_str());

      // get pos for stream info
      ImGui::SameLine();
      ImVec2 pos = ImGui::GetCursorPos();
      pos.y -= ImGui::GetScrollY();

      if (pidInfo.mStreamType == 6) {
        //{{{  draw subtitle
        cSubtitle* subtitle = dvbTransportStream->getSubtitleBySid (pidInfo.mSid);

        if (subtitle && !subtitle->mRects.empty()) {
          float clutPotX = ImGui::GetWindowWidth() - ImGui::GetTextLineHeight() * 5.f;
          float clutPotSize = ImGui::GetTextLineHeight()/2.f;

          ImVec2 subtitlePos = pos;
          for (int line = (int)subtitle->mRects.size()-1; line >= 0; line--) {
            float dstWidth = ImGui::GetWindowWidth() - pos.x;
            float dstHeight = float(subtitle->mRects[line]->mHeight * dstWidth) / subtitle->mRects[line]->mWidth;
            if (dstHeight > ImGui::GetTextLineHeight()) {
              // scale to fit window
              float scaleh = ImGui::GetTextLineHeight() / dstHeight;
              dstHeight = ImGui::GetTextLineHeight();
              dstWidth *= scaleh;
              }

            // create or update rect image
            //if (mImage[imageIndex] == -1) {
            //  if (imageIndex < 20)
            //    mImage[imageIndex] = vg->createImageRGBA (
            //      subtitle->mRects[line]->mWidth, subtitle->mRects[line]->mHeight, 0, (uint8_t*)subtitle->mRects[line]->mPixData);
            //  else
            //    cLog::log (LOGERROR, "too many cDvbWidget images, fixit");
            //  }
            //else if (subtitle->mChanged)  // !!! assumes image is same size as before !!!
            //  vg->updateImage (mImage[imageIndex], (uint8_t*)subtitle->mRects[line]->mPixData);
            //auto imagePaint = vg->setImagePattern (cPointF(visx, ySub), cPointF(dstWidth, dstHeight), 0.f, mImage[imageIndex], 1.f);
            ImGui::GetWindowDrawList()->AddRect (
              subtitlePos, {subtitlePos.x + dstWidth, subtitlePos.y + dstHeight}, 0xff00ffff);
            //imageIndex++;

            // draw subtitle position
            string text = fmt::format ("{},{}", subtitle->mRects[line]->mX, subtitle->mRects[line]->mY);
            ImGui::GetWindowDrawList()->AddText (
              ImGui::GetFont(), ImGui::GetFontSize(), subtitlePos, 0xffffffff, text.c_str());

            // draw clut color pots
            for (int pot = 0; pot < subtitle->mRects[line]->mClutSize; pot++) {
              ImVec2 clutPotPos {clutPotX + (pot % 8) * clutPotSize, subtitlePos.y + (pot / 8) * clutPotSize};
              uint32_t color = subtitle->mRects[line]->mClut[pot]; // possible swizzle
              ImGui::GetWindowDrawList()->AddRectFilled (
                clutPotPos, {clutPotPos.x + clutPotSize - 1.f, clutPotPos.y + clutPotSize - 1.f}, color);
              }

            // next subtitle line
            subtitlePos.y += ImGui::GetTextLineHeight();
            }

          // reset changed flag
          subtitle->mChanged = false;
          }
        }
        //}}}

      // draw stream bar
      gMaxPidPackets = max (gMaxPidPackets, pidInfo.mPackets);
      float frac = pidInfo.mPackets / float(gMaxPidPackets);
      ImVec2 posTo = {pos.x + (frac * (ImGui::GetWindowWidth() - pos.x)),
                      pos.y + ImGui::GetTextLineHeight()};
      ImGui::GetWindowDrawList()->AddRectFilled (pos, posTo, 0xff00ffff);

      // draw stream text
      string streamText = pidInfo.getInfoString();
      if ((pidInfo.mStreamType == 0) && (pidInfo.mSid != 0xFFFF))
        streamText = fmt::format ("{} ", pidInfo.mSid) + streamText;
      ImGui::TextUnformatted (streamText.c_str());

      // width of packet field
      if (pidInfo.mPackets > pow (10, gPacketDigits))
        gPacketDigits++;

      prevSid = pidInfo.mSid;
      }
    }
  //}}}
  }

class cTvUI : public cUI {
public:
  cTvUI (const string& name) : cUI(name) {}
  virtual ~cTvUI() = default;

  //{{{
  void addToDrawList (cApp& app) final {

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

    ImGui::Begin ("player", &mOpen, ImGuiWindowFlags_NoTitleBar);
    //{{{  draw top buttons
    // vsync button,fps
    if (app.getPlatform().hasVsync()) {
      // vsync button
      if (toggleButton ("vSync", app.getPlatform().getVsync()))
        app.getPlatform().toggleVsync();

      // fps text
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      ImGui::SameLine();
      }

    // fullScreen button
    if (app.getPlatform().hasFullScreen()) {
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      ImGui::SameLine();
      }

    // vertice debug
    ImGui::TextUnformatted (fmt::format ("{}:{}",
                            ImGui::GetIO().MetricsRenderVertices,
                            ImGui::GetIO().MetricsRenderIndices/3).c_str());

    //}}}

    cTvApp& tvApp = (cTvApp&)app;
    cDvbTransportStream* dvbTransportStream = tvApp.getDvbTransportStream();
    if (dvbTransportStream) {
      ImGui::SameLine();
      if (toggleButton ("sub", tvApp.getSubtitle()))
        tvApp.toggleSubtitle();

      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format ("{} ", dvbTransportStream->getNumPackets()).c_str());
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format("{} ", dvbTransportStream->getNumErrors()).c_str());
      ImGui::SameLine();
      ImGui::TextUnformatted (dvbTransportStream->getSignalString().c_str());
      ImGui::SameLine();
      ImGui::TextUnformatted (dvbTransportStream->getErrorString().c_str());

      ImGui::PushFont (app.getMonoFont());
      drawTransportStream (dvbTransportStream);
      ImGui::PopFont();
      }

    ImGui::End();
    }
  //}}}

private:
  // vars
  bool mOpen = true;

  static cUI* create (const string& className) { return new cTvUI (className); }
  inline static const bool mRegistered = registerClass ("tv", &create);
  };

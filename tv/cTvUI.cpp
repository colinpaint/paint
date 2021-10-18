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
#include "../dvb/cTsDvb.h"
#include "../dvb/cTransportStream.h"
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
  uint32_t gErrorDigits = 0;
  uint32_t gPacketDigits = 0;
  int64_t gMaxPidPackets = 0;
  //{{{
  void drawPids (cTsDvb& tsDvb) {

    // width of error field
    if (tsDvb.getTransportStream()->getErrors() > pow (10, gErrorDigits)) 
      gErrorDigits++;

    int prevSid = 0;
    for (auto& pidInfoItem : tsDvb.getTransportStream()->mPidInfoMap) {
      // iterate pidInfo
      cPidInfo& pidInfo = pidInfoItem.second;
      if ((pidInfo.mSid != prevSid) && (pidInfo.mStreamType != 5) && (pidInfo.mStreamType != 11))
        ImGui::Separator();

      ImGui::TextUnformatted (fmt::format ("{:{}d} {:{}d} {:4d} {} {}",
                              pidInfo.mPackets, gPacketDigits, pidInfo.mErrors, gErrorDigits, pidInfo.mPid,
                              getFullPtsString (pidInfo.mPts), pidInfo.getTypeString()).c_str());

      if (pidInfo.mStreamType == 6) {
        //{{{  draw subtitle
        //cSubtitle* subtitle = mDvb->getSubtitleBySid (pidInfo.mSid);

        //if (subtitle) {
          //if (!subtitle->mRects.empty()) {
            //// subttitle with some rects
            //float ySub = y - getLineHeight();

            //for (int line = (int)subtitle->mRects.size()-1; line >= 0; line--) {
              //// iterate rects
              //float dstWidth = mSize.x - visx;
              //float dstHeight = float(subtitle->mRects[line]->mHeight * dstWidth) / subtitle->mRects[line]->mWidth;
              //if (dstHeight > getLineHeight()) {
                //// scale to fit line
                //float scaleh = getLineHeight() / dstHeight;
                //dstHeight = getLineHeight();
                //dstWidth *= scaleh;
                //}

              //// draw bgnd
              //draw->drawRect (kDarkGreyF, cPointF(visx, ySub), cPointF(dstWidth, dstHeight));

              //// create or update rect image
              //if (mImage[imageIndex] == -1) {
                //if (imageIndex < 20)
                  //mImage[imageIndex] = vg->createImageRGBA (
                    //subtitle->mRects[line]->mWidth, subtitle->mRects[line]->mHeight, 0, (uint8_t*)subtitle->mRects[line]->mPixData);
                //else
                  //cLog::log (LOGERROR, "too many cDvbWidget images, fixit");
                //}
              //else if (subtitle->mChanged)  // !!! assumes image is same size as before !!!
                //vg->updateImage (mImage[imageIndex], (uint8_t*)subtitle->mRects[line]->mPixData);

              //// draw rect image
              //auto imagePaint = vg->setImagePattern (cPointF(visx, ySub), cPointF(dstWidth, dstHeight), 0.f, mImage[imageIndex], 1.f);
              //vg->beginPath();
              //vg->rect (cPointF(visx, ySub), cPointF(dstWidth, dstHeight));
              //vg->setFillPaint (imagePaint);
              //vg->fill();

              //imageIndex++;

              //// draw rect position
              //std::string text = dec(subtitle->mRects[line]->mX) + "," + dec(subtitle->mRects[line]->mY,3);
              //float posWidth = draw->drawTextRight (kWhiteF, getLineHeight(), text, cPointF(mOrg.x + mSize.x,  ySub), cPointF(mSize.x - mOrg.x, dstHeight));

              //// draw clut
              //float clutX = mSize.x - mOrg.x - posWidth - getLineHeight() * 4.f;
              //for (int i = 0; i < subtitle->mRects[line]->mClutSize; i++) {
                //float cx = clutX + (i % 8) * getLineHeight() / 2.f;
                //float cy = ySub + (i / 8) * getLineHeight() / 2.f;
                //draw->drawRect (sColourF(subtitle->mRects[line]->mClut[i]), cPointF(cx, cy), cPointF((getLineHeight()/2.f)-1.f, (getLineHeight() / 2.f) - 1.f));
                //}

              //// next subtitle line
              //ySub += getLineHeight();
              //}
            //}

          //// reset changed flag
          //subtitle->mChanged = false;
          //}
        //}
        //}}}
        }

      // get pos for stream info
      ImGui::SameLine();
      ImVec2 pos = ImGui::GetCursorPos();
      pos.y -= ImGui::GetScrollY();

      // draw stream bar
      gMaxPidPackets = max (gMaxPidPackets, pidInfo.mPackets);
      float frac = pidInfo.mPackets / float(gMaxPidPackets);
      ImVec2 posTo = {pos.x + (frac * (ImGui::GetWindowWidth() - pos.x)), pos.y + ImGui::GetTextLineHeight() - 1.f};
      ImGui::GetWindowDrawList()->AddRectFilled (pos, posTo, 0xff00ffff);

      // draw stream text
      string streamText = pidInfo.getInfoString();
      if ((pidInfo.mStreamType == 0) && (pidInfo.mSid > 0))
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
  //{{{
    cTvUI(const string& name) : cUI(name) {
    }
  //}}}
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
      ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      ImGui::SameLine();
      }

    // fullScreen button
    if (app.getPlatform().hasFullScreen()) {
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      ImGui::SameLine();
      }

    // vertice debug
    ImGui::Text (fmt::format ("{}:{}",
                 ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
    //}}}

    ImGui::PushFont (app.getMonoFont());
    cTvApp& tvApp = (cTvApp&)app;
    drawPids (tvApp.getTsDvb());
    ImGui::PopFont();

    ImGui::End();
    }
  //}}}

private:
  // vars
  bool mOpen = true;

  static cUI* create (const string& className) { return new cTvUI (className); }
  inline static const bool mRegistered = registerClass ("tv", &create);
  };

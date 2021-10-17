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

//{{{
class cDrawTv : public cDrawContext {
public:
  cDrawTv() : cDrawContext (kPalette) {}
  //{{{
  void draw() {

    update (18.f, true);
    layout();

    int lastSid = 0;
    //int imageIndex = 0;
    float x = 2.f;
    float y = getLineHeight() * 2;
    for (auto& pidInfoItem : mDvb->getTransportStream()->mPidInfoMap) {
      // iterate pidInfo
      cPidInfo& pidInfo = pidInfoItem.second;
      int pid = pidInfo.mPid;

      if ((pidInfo.mSid != lastSid) && (pidInfo.mStreamType != 5) && (pidInfo.mStreamType != 11))
        rect ({x,y}, {mSize.x, y + 1.f}, eServiceLine);

      float textWidth = text ({x,y}, eText, fmt::format ("{:{}d} {:{}d} {:4d} {} {}",
                                                         pidInfo.mPackets, mPacketDigits,
                                                         pidInfo.mErrors, mContDigits,
                                                         pid, 
                                                         getFullPtsString (pidInfo.mPts),
                                                         pidInfo.getTypeString()));
      float visx = x + textWidth + getLineHeight()/2.f;

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

      mMaxPidPackets = max (mMaxPidPackets, (float)pidInfo.mPackets);
      float frac = pidInfo.mPackets / mMaxPidPackets;
      rect ({visx, y}, {visx + (frac * (mSize.x - textWidth)), y + getLineHeight() - 1.f}, eBar);

      string streamString = pidInfo.getInfoString();
      if ((pidInfo.mStreamType == 0) && (pidInfo.mSid > 0))
        streamString = fmt::format("{} ", pidInfo.mSid) + streamString;
      text ({visx,  y}, eText, streamString);

      if (pidInfo.mPackets > pow (10, mPacketDigits))
        mPacketDigits++;

      lastSid = pidInfo.mSid;
      y += getLineHeight();
      }
    mLastHeight = y;

    if (mDvb->getTransportStream()->getErrors() > pow (10, mContDigits))
      mContDigits++;
    }
  //}}}

private:
  //{{{  palette const
  inline static const uint8_t eBackground =  0;
  inline static const uint8_t eText =        1;
  inline static const uint8_t eBar =         2;
  inline static const uint8_t eServiceLine = 3;

  inline static const vector <ImU32> kPalette = {
  //  aabbggrr
    0xff000000, // eBackground
    0xffffffff, // eText
    0xff00ffff, // eBar
    0xff606060, // eServiceLine
    };
  //}}}
  //{{{
  void layout() {

    // check for window size change, refresh any caches dependent on size
    ImVec2 size = ImGui::GetWindowSize();
    mChanged |= (size.x != mSize.x) || (size.y != mSize.y);
    mSize = size;
    }
  //}}}

  // vars
  bool mChanged = false;
  ImVec2 mSize = {0.f,0.f};

  cTsDvb* mDvb;
  int mContDigits = 0;
  int mPacketDigits = 1;
  float mMaxPidPackets = 0;

  float mZoom  = 0.f;
  float mLastHeight = 0.f;

  int mImage[20] =  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1 };

  };
//}}}

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

    ImGui::Begin ("player", &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);
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
    mDrawTv.draw();
    ImGui::PopFont();

    ImGui::End();
    }
  //}}}

private:
  // vars
  bool mOpen = true;
  cDrawTv mDrawTv;

  static cUI* create (const string& className) { return new cTvUI (className); }
  inline static const bool mRegistered = registerClass ("tv", &create);
  };

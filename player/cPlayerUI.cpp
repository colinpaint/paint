// cPlayerUI.cpp - will extend to deal with many file types invoking best editUI
//{{{  includes
#include <cstdint>
#include <array>
#include <vector>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImguiWidgets.h"

// ui
#include "../ui/cUI.h"

#include "../platform/cPlatform.h"
#include "../graphics/cGraphics.h"

// decoder
#include "../utils/cFileView.h"

// song
#include "../song/cSongLoader.h"
#include "../song/cSong.h"

// utils
#include "../utils/cLog.h"
#include "../utils/date.h"

using namespace std;
//}}}
//{{{  const
static const vector<string> kRadio1 = {"r1", "a128"};
static const vector<string> kRadio2 = {"r2", "a128"};
static const vector<string> kRadio3 = {"r3", "a320"};
static const vector<string> kRadio4 = {"r4", "a64"};
static const vector<string> kRadio5 = {"r5", "a128"};
static const vector<string> kRadio6 = {"r6", "a128"};

static const vector<string> kBbc1   = {"bbc1", "a128"};
static const vector<string> kBbc2   = {"bbc2", "a128"};
static const vector<string> kBbc4   = {"bbc4", "a128"};
static const vector<string> kNews   = {"news", "a128"};
static const vector<string> kBbcSw  = {"sw", "a128"};

static const vector<string> kWqxr  = {"http://stream.wqxr.org/js-stream.aac"};
static const vector<string> kDvb  = {"dvb"};

static const vector<string> kRtp1  = {"rtp 1"};
static const vector<string> kRtp2  = {"rtp 2"};
static const vector<string> kRtp3  = {"rtp 3"};
static const vector<string> kRtp4  = {"rtp 4"};
static const vector<string> kRtp5  = {"rtp 5"};
//}}}
//{{{  palette const
constexpr uint8_t eBackground =  0;
constexpr uint8_t eText =        1;
constexpr uint8_t eFreq =        2;
constexpr uint8_t ePeak =        3;
constexpr uint8_t ePowerPlayed = 4;
constexpr uint8_t ePowerPlay =   5;
constexpr uint8_t ePowerToPlay = 6;

constexpr uint8_t eUndefined = 0xFF;

// color to ImU32 lookup
const vector <ImU32> kPalette = {
//  aabbggrr
  0xff000000, // eBackground
  0xffffffff, // eText
  0xff00ffff, // eFreq
  0xff606060, // ePeak
  0xffc00000, // ePowerPlayed
  0xffffffff, // ePowerPlay
  0xff808080, // ePowerToPlay
  };
//}}}

//class cSongWidget {
 //void setShowOverview (bool showOverview) { mShowOverview = showOverview; }
  //void toggleGraphics() { mShowGraphics = ! mShowGraphics; }
  //{{{
  //void layout() {

    //mOverviewTotalFrames = 0;
    //mOverviewLastFrame = 0;
    //mOverviewFirstFrame = 0;
    //mOverviewValueScale = 0.f;
    //}
  //}}}

  //{{{
  //void onDown (cPointF point) {

    //cWidget::onDown (point);

    //cSong* song = mLoader->getSong();

    //if (song) {
      ////std::shared_lock<std::shared_mutex> lock (song->getSharedMutex());
      //if (point.y > mDstOverviewTop) {
        //int64_t frameNum = song->getFirstFrameNum() + int((point.x * song->getTotalFrames()) / getWidth());
        //song->setPlayPts (song->getPtsFromFrameNum (frameNum));
        //mOverviewPressed = true;
        //}

      //else if (point.y > mDstRangeTop) {
        //mPressedFrameNum = song->getPlayFrameNum() + ((point.x - (getWidth()/2.f)) * mFrameStep / mFrameWidth);
        //song->getSelect().start (int64_t(mPressedFrameNum));
        //mRangePressed = true;
        ////mWindow->changed();
        //}

      //else
        //mPressedFrameNum = double(song->getFrameNumFromPts (int64_t(song->getPlayPts())));
      //}
    //}
  //}}}
  //{{{
  //void onMove (cPointF point, cPointF inc) {

    //cWidget::onMove (point, inc);

    //cSong* song = mLoader->getSong();
    //if (song) {
      ////std::shared_lock<std::shared_mutex> lock (song.getSharedMutex());
      //if (mOverviewPressed)
        //song->setPlayPts (
          //song->getPtsFromFrameNum (
            //song->getFirstFrameNum() + int64_t(point.x * song->getTotalFrames() / getWidth())));

      //else if (mRangePressed) {
        //mPressedFrameNum += (inc.x / mFrameWidth) * mFrameStep;
        //song->getSelect().move (int64_t(mPressedFrameNum));
        ////mWindow->changed();
        //}

      //else {
        //mPressedFrameNum -= (inc.x / mFrameWidth) * mFrameStep;
        //song->setPlayPts (song->getPtsFromFrameNum (int64_t(mPressedFrameNum)));
        //}
      //}
    //}
  //}}}
  //{{{
  //void onUp() {

    //cWidget::onUp();

    //cSong* song = mLoader->getSong();
    //if (song)
      //song->getSelect().end();

    //mOverviewPressed = false;
    //mRangePressed = false;
    //}
  //}}}
  //{{{
  //void onWheel (float delta) {

    ////if (getShow())
    //setZoom (mZoom - (int)delta);
    //}
  //}}}

  //{{{
  //void setZoom (int zoom) {

    //mZoom = std::min (std::max (zoom, mMinZoom), mMaxZoom);

    //// zoomIn expanding frame to mFrameWidth pix
    //mFrameWidth = (mZoom < 0) ? -mZoom+1 : 1;

    //// zoomOut summing mFrameStep frames per pix
    //mFrameStep = (mZoom > 0) ? mZoom+1 : 1;
    //}
  //}}}

  // draws
  //{{{
  //void drawSpinner (cVg* vg, cPointF centre, float outerRadius, float innerRadius,
                    //float fracFrom, float fracTo,
                    //const sColourF& colourFrom, const sColourF& colourTo) {

    //if ((fracTo - fracFrom)  > 0.f) {
      //float angleFrom = -kPiDiv2 + (fracFrom * k2Pi);
      //float angleTo = -kPiDiv2 + (fracTo * k2Pi);

      //vg->beginPath();
      //vg->arc (centre, outerRadius, angleFrom, angleTo, cVg::eClockWise);
      //vg->arc (centre, innerRadius, angleTo, angleFrom, cVg::eCounterClockWise);
      //vg->closePath();

      //float midRadius = (outerRadius + innerRadius) / 2.f;
      //vg->setFillPaint (vg->setLinearGradient (centre + cPointF (cosf (angleFrom), sinf (angleFrom)) * midRadius,
                                               //centre + cPointF (cosf (angleTo), sinf (angleTo)) * midRadius,
                                               //colourFrom, colourTo));
      //vg->fill();
      //}
    //}
  //}}}
  //{{{
  //void drawVideoPool (cVg* vg, cSong* song, iVideoPool* videoPool, int64_t playerPts) {

    //cPointF org { getCentre().x, mDstWaveCentre };
    //const float ptsPerPix = float((90 * song->getSamplesPerFrame()) / 48);
    //constexpr float kPesSizeScale = 1000.f;

    //{{{  draw B frames yellow
    //vg->beginPath();

    //for (auto frame : videoPool->getFramePool()) {
      //if (!frame.second->isFree() && (frame.second->getFrameType() == 'B'))  {
        //float pix = floor ((frame.second->getPts() - playerPts) / ptsPerPix);
        //float pes = frame.second->getPesSize() / kPesSizeScale;
        //vg->rect (org + cPointF (pix, -pes), cPointF(1.f, pes));
        //}
      //}

    //vg->setFillColour (kYellowF);
    //vg->triangleFill();
    //}}}
    //{{{  draw P frames cyan
    //vg->beginPath();

    //for (auto frame : videoPool->getFramePool()) {
      //if (!frame.second->isFree() && (frame.second->getFrameType() == 'P'))  {
        //float pix = floor ((frame.second->getPts() - playerPts) / ptsPerPix);
        //float pes = frame.second->getPesSize() / kPesSizeScale;
        //vg->rect (org + cPointF (pix, -pes), cPointF(1.f, pes));
        //}
      //}

    //vg->setFillColour (kCyanF);
    //vg->triangleFill();
    //}}}
    //{{{  draw I frames white
    //vg->beginPath();

    //for (auto frame : videoPool->getFramePool()) {
      //if (!frame.second->isFree() && (frame.second->getFrameType() == 'I'))  {
        //float pix = floor ((frame.second->getPts() - playerPts) / ptsPerPix);
        //float pes = frame.second->getPesSize() / kPesSizeScale;
        //vg->rect (org + cPointF (pix, -pes), cPointF(1.f, pes));
        //}
      //}

    //vg->setFillColour (kWhiteF);
    //vg->triangleFill();
    //}}}
    //}
  //}}}
//  };

class cPlayerUI : public cUI {
public:
  //{{{
  cPlayerUI (const string& name) : cUI(name) {
    }
  //}}}
  //{{{
  virtual ~cPlayerUI() {
    // close the file mapping object
    }
  //}}}

  void addToDrawList (cApp& app) final {

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    bool mOpen;
    ImGui::Begin ("player", &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

    if (!mFileLoaded) {
      //{{{  load file
      const vector <string>& strings = { app.getName() };
      if (!app.getName().empty()) {
        mSongLoader.launchLoad (strings);
        mFileLoaded = true;
        }
      }
      //}}}

    drawContents (app);

    ImGui::End();
    }

private:
  bool isDrawMonoSpaced() { return mShowMonoSpaced; }
  void toggleShowMonoSpaced() { mShowMonoSpaced = ! mShowMonoSpaced; }

  //{{{
  void drawRange (cSong* song, int64_t playFrame, int64_t leftFrame, int64_t rightFrame) {

    //vg->beginPath();
    //vg->rect (cPointF(0.f, mDstRangeTop), cPointF(mSize.x, mRangeHeight));
    //vg->setFillColour (kDarkGreyF);
    //vg->triangleFill();

    //vg->beginPath();
    //for (auto &item : song->getSelect().getItems()) {
    //  auto firstx = (mSize.y/ 2.f) + (item.getFirstFrame() - playFrame) * mFrameWidth / mFrameStep;
    //  float lastx = item.getMark() ? firstx + 1.f :
    //                                 (getWidth()/2.f) + (item.getLastFrame() - playFrame) * mFrameWidth / mFrameStep;
    //  vg->rect (cPointF(firstx, mDstRangeTop), cPointF(lastx - firstx, mRangeHeight));

    //  auto title = item.getTitle();
    //  if (!title.empty()) {
        //dstRect = { mRect.left + firstx + 2.f, mDstRangeTop + mRangeHeight - mWindow->getTextFormat()->GetFontSize(),
        //            mRect.right, mDstRangeTop + mRangeHeight + 4.f };
        //dc->DrawText (std::wstring (title.begin(), title.end()).data(), (uint32_t)title.size(), mWindow->getTextFormat(),
        //              dstRect, mWindow->getWhiteBrush(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
     //   }
      //}

    //vg->setFillColour (kWhiteF);
    //vg->triangleFill();
    }
  //}}}
  //{{{
  void drawWave (cSong* song, int64_t playFrame, int64_t leftFrame, int64_t rightFrame, bool mono) {

    array <float,2> values = { 0.f };

    float peakValueScale = mWaveHeight / song->getMaxPeakValue() / 2.f;
    float powerValueScale = mWaveHeight / song->getMaxPowerValue() / 2.f;

    float xlen = (float)mFrameStep;
    if (mFrameStep == 1) {
      //{{{  draw all peak values
      float xorg = 0;
      for (int64_t frame = leftFrame; frame < rightFrame; frame += mFrameStep) {
        cSong::cFrame* framePtr = song->findFrameByFrameNum (frame);
        if (framePtr) {
          // draw frame peak values scaled to maxPeak
          if (framePtr->getPowerValues()) {
            float* peakValuesPtr = framePtr->getPeakValues();
            for (size_t i = 0; i < 2; i++)
              values[i] = *peakValuesPtr++ * peakValueScale;
            }
          mDrawContext.rect ({xorg, mDstWaveCentre - values[0]}, {xorg + xlen, mDstWaveCentre + values[1]}, ePeak);
          }
        xorg += xlen;
        }
      }
      //}}}

    float xorg = 0;
    //{{{  draw powerValues before playFrame, summed if zoomed out, ePowerPlayed
    for (auto frame = leftFrame; frame < playFrame; frame += mFrameStep) {
      auto framePtr = song->findFrameByFrameNum (frame);
      if (framePtr) {
        if (mFrameStep == 1) {
          // power scaled to maxPeak
          if (framePtr->getPowerValues()) {
           auto powerValuesPtr = framePtr->getPowerValues();
            for (auto i = 0; i < 2; i++)
              values[i] = *powerValuesPtr++ * peakValueScale;
            }
          }
        else {
          // sum mFrameStep frames, mFrameStep aligned, scaled to maxPower
          for (auto i = 0; i < 2; i++)
            values[i] = 0.f;

          auto alignedFrame = frame - (frame % mFrameStep);
          auto toSumFrame = min (alignedFrame + mFrameStep, rightFrame);
          for (auto sumFrame = alignedFrame; sumFrame < toSumFrame; sumFrame++) {
            auto framePtr = song->findFrameByFrameNum (sumFrame);
            if (framePtr) {
              if (framePtr->getPowerValues()) {
                auto powerValuesPtr = framePtr->getPowerValues();
                for (auto i = 0; i < 2; i++)
                  values[i] += *powerValuesPtr++ * powerValueScale;
                }
              }
            }

          for (auto i = 0; i < 2; i++)
            values[i] /= toSumFrame - alignedFrame + 1;
          }
        mDrawContext.rect ({xorg, mDstWaveCentre - values[0]},
                           {xorg + xlen, mDstWaveCentre + values[1]}, ePowerPlayed);
        }

      xorg += xlen;
      }
    //}}}
    //{{{  draw powerValues playFrame, no sum, ePowerPlay
    // power scaled to maxPeak
    cSong::cFrame* framePtr = song->findFrameByFrameNum (playFrame);
    if (framePtr) {
      //  draw play frame power scaled to maxPeak
      if (framePtr->getPowerValues()) {
        float* powerValuesPtr = framePtr->getPowerValues();
        for (size_t i = 0; i < 2; i++)
          values[i] = *powerValuesPtr++ * peakValueScale;
        }

      mDrawContext.rect ({xorg, mDstWaveCentre - values[0]},
                         {xorg + xlen, mDstWaveCentre + values[1]}, ePowerPlay);
      }

    xorg += xlen;
    //}}}
    //{{{  draw powerValues after playFrame, summed if zoomed out, ePowerToPlay
    for (int64_t frame = playFrame+mFrameStep; frame < rightFrame; frame += mFrameStep) {
      cSong::cFrame* framePtr = song->findFrameByFrameNum (frame);
      if (framePtr) {
        if (mFrameStep == 1) {
          // power scaled to maxPeak
          if (framePtr->getPowerValues()) {
            float* powerValuesPtr = framePtr->getPowerValues();
            for (size_t i = 0; i < 2; i++)
              values[i] = *powerValuesPtr++ * peakValueScale;
            }
          }
        else {
          // sum mFrameStep frames, mFrameStep aligned, scaled to maxPower
          for (size_t i = 0; i < 2; i++)
            values[i] = 0.f;

          int64_t alignedFrame = frame - (frame % mFrameStep);
          int64_t toSumFrame = min (alignedFrame + mFrameStep, rightFrame);
          for (int64_t sumFrame = alignedFrame; sumFrame < toSumFrame; sumFrame++) {
            cSong::cFrame* framePtr = song->findFrameByFrameNum (sumFrame);
            if (framePtr) {
              if (framePtr->getPowerValues()) {
                float* powerValuesPtr = framePtr->getPowerValues();
                for (size_t i = 0; i < 2; i++)
                  values[i] += *powerValuesPtr++ * powerValueScale;
                }
              }
            }

          for (size_t i = 0; i < 2; i++)
            values[i] /= toSumFrame - alignedFrame + 1;
          }

        mDrawContext.rect ({xorg, mDstWaveCentre - values[0]},
                           {xorg + xlen, mDstWaveCentre + values[1]}, ePowerToPlay);
        }

      xorg += xlen;
      }
    //}}}

    //{{{  copy reversed spectrum column to bitmap, clip high freqs to height
    //int freqSize = min (song->getNumFreqBytes(), (int)mFreqHeight);
    //int freqOffset = song->getNumFreqBytes() > (int)mFreqHeight ? song->getNumFreqBytes() - (int)mFreqHeight : 0;

    // bitmap sampled aligned to mFrameStep, !!! could sum !!! ?? ok if neg frame ???
    //auto alignedFromFrame = fromFrame - (fromFrame % mFrameStep);
    //for (auto frame = alignedFromFrame; frame < toFrame; frame += mFrameStep) {
      //auto framePtr = song->getAudioFramePtr (frame);
      //if (framePtr) {
        //if (framePtr->getFreqLuma()) {
          //uint32_t bitmapIndex = getSrcIndex (frame);
          //D2D1_RECT_U bitmapRectU = { bitmapIndex, 0, bitmapIndex+1, (UINT32)freqSize };
          //mBitmap->CopyFromMemory (&bitmapRectU, framePtr->getFreqLuma() + freqOffset, 1);
          //}
        //}
      //}
    //}}}
    }
  //}}}
  //{{{
  void drawFreq (cSong* song, int64_t playFrame) {

    float valueScale = 100.f / 255.f;

    float xorg = 0.f;
    auto framePtr = song->findFrameByFrameNum (playFrame);
    if (framePtr && framePtr->getFreqValues()) {
      auto freqValues = framePtr->getFreqValues();
      for (size_t i = 0; (i < song->getNumFreqBytes()) && ((i*2) < int(mSize.x)); i++) {
        float value =  freqValues[i] * valueScale;
        if (value > 1.f)
          mDrawContext.rect ({xorg, mSize.y - value}, {xorg + 2.f, 0 + mSize.y}, eFreq);
        xorg += 2.f;
        }
      }
    }
  //}}}
  //{{{
  void drawOverviewWave (cSong* song, int64_t firstFrame, int64_t playFrame, float playFrameX, float valueScale, bool mono) {
  // simple overview cache, invalidate if anything changed

    int64_t lastFrame = song->getLastFrameNum();
    int64_t totalFrames = song->getTotalFrames();

    bool changed = (mOverviewTotalFrames != totalFrames) ||
                   (mOverviewLastFrame != lastFrame) ||
                   (mOverviewFirstFrame != firstFrame) ||
                   (mOverviewValueScale != valueScale);

    float values[2] = { 0.f };

    //vg->beginPath();
    float xorg = 0;
    float xlen = 1.f;
    for (auto x = 0; x < int(mSize.x); x++) {
      // iterate widget width
      if (changed) {
        int64_t frame = firstFrame + ((x * totalFrames) / int(mSize.x));
        int64_t toFrame = firstFrame + (((x+1) * totalFrames) / int(mSize.x));
        if (toFrame > lastFrame)
          toFrame = lastFrame+1;

        auto framePtr = song->findFrameByFrameNum (frame);
        if (framePtr && framePtr->getPowerValues()) {
          // accumulate frame, handle silence better
          float* powerValues = framePtr->getPowerValues();
          values[0] = powerValues[0];
          values[1] = mono ? 0 : powerValues[1];

          if (frame < toFrame) {
            int numSummedFrames = 1;
            frame++;
            while (frame < toFrame) {
              framePtr = song->findFrameByFrameNum (frame);
              if (framePtr) {
                if (framePtr->getPowerValues()) {
                  auto powerValues = framePtr->getPowerValues();
                  values[0] += powerValues[0];
                  values[1] += mono ? 0 : powerValues[1];
                  numSummedFrames++;
                  }
                }
              frame++;
              }
            values[0] /= numSummedFrames;
            values[1] /= numSummedFrames;
            }
          values[0] *= valueScale;
          values[1] *= valueScale;
          mOverviewValuesL[x] = values[0];
          mOverviewValuesR[x] = values[0] + values[1];
          }
        }

      //vg->rect (cPointF(xorg, 0 + mDstOverviewCentre - mOverviewValuesL[x]), cPointF(xlen,  mOverviewValuesR[x]));
      xorg += 1.f;
      }
    //vg->setFillColour (kGreyF);
    //vg->triangleFill();

    // possible cache to stop recalc
    mOverviewTotalFrames = totalFrames;
    mOverviewLastFrame = lastFrame;
    mOverviewFirstFrame = firstFrame;
    mOverviewValueScale = valueScale;
    }
  //}}}
  //{{{
  void drawOverviewLens (cSong* song, int64_t playFrame, float centreX, float width, bool mono) {
  // draw frames centred at playFrame -/+ width in els, centred at centreX

    cLog::log (LOGINFO, "drawOverviewLens %d %f %f", playFrame, centreX, width);

    // cut hole and frame it
    //vg->beginPath();
    //vg->rect (0 + cPointF(centreX - width, mDstOverviewTop), cPointF(width * 2.f, mOverviewHeight));
    //vg->setFillColour (kBlackF);
    //vg->triangleFill();
    // frame in yellow

    // calc leftmost frame, clip to valid frame, adjust firstX which may overlap left up to mFrameWidth
    float leftFrame = playFrame - width;
    float firstX = centreX - (playFrame - leftFrame);
    if (leftFrame < 0) {
      firstX += -leftFrame;
      leftFrame = 0;
      }

    int64_t rightFrame = playFrame + (int64_t)width;
    rightFrame = min (rightFrame, song->getLastFrameNum());

    // calc lens max power
    float maxPowerValue = 0.f;
    for (auto frame = int(leftFrame); frame <= rightFrame; frame++) {
      auto framePtr = song->findFrameByFrameNum (frame);
      if (framePtr && framePtr->getPowerValues()) {
        auto powerValues = framePtr->getPowerValues();
        maxPowerValue = max (maxPowerValue, powerValues[0]);
        if (!mono)
          maxPowerValue = max (maxPowerValue, powerValues[1]);
        }
      }

    // draw unzoomed waveform, start before playFrame
    //vg->beginPath();
    float xorg = firstX;
    float valueScale = mOverviewHeight / 2.f / maxPowerValue;
    for (auto frame = int(leftFrame); frame <= rightFrame; frame++) {
      auto framePtr = song->findFrameByFrameNum (frame);
      if (framePtr && framePtr->getPowerValues()) {
        //if (framePtr->hasTitle()) {
          //{{{  draw song title yellow bar and text
          //cRect barRect = { dstRect.left-1.f, mDstOverviewTop, dstRect.left+1.f, mRect.bottom };
          //dc->FillRectangle (barRect, mWindow->getYellowBrush());

          //auto str = framePtr->getTitle();
          //dc->DrawText (wstring (str.begin(), str.end()).data(), (uint32_t)str.size(), mWindow->getTextFormat(),
                        //{ dstRect.left+2.f, mDstOverviewTop, getWidth(), mDstOverviewTop + mOverviewHeight },
                        //mWindow->getWhiteBrush());
          //}
          //}}}
        //if (framePtr->isSilence()) {
          //{{{  draw red silence
          //dstRect.top = mDstOverviewCentre - 2.f;
          //dstRect.bottom = mDstOverviewCentre + 2.f;
          //dc->FillRectangle (dstRect, mWindow->getRedBrush());
          //}
          //}}}

        if (frame == playFrame) {
          //{{{  finish before playFrame
          //vg->setFillColour (kBlueF);
          //vg->triangleFill();
          //vg->beginPath();
          }
          //}}}

        auto powerValues = framePtr->getPowerValues();
        float yorg = mono ? mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f) :
                            mDstOverviewCentre - (powerValues[0] * valueScale);
        float ylen = mono ? powerValues[0] * valueScale * 2.f  :
                            (powerValues[0] + powerValues[1]) * valueScale;
        //vg->rect (cPointF(xorg, 0 + yorg), cPointF(1.f, ylen));

        if (frame == playFrame) {
          //{{{  finish playFrame, start after playFrame
          //vg->setFillColour (kWhiteF);
          //vg->triangleFill();
          //vg->beginPath();
          }
          //}}}
        }

      xorg += 1.f;
      }
    // finish after playFrame
    //vg->setFillColour (kGreyF);
    //vg->triangleFill();
    }
  //}}}
  //{{{
  void drawOverview (cSong* song, int64_t playFrame, bool mono) {

    if (!song->getTotalFrames())
      return;

    int64_t firstFrame = song->getFirstFrameNum();
    float playFrameX = ((playFrame - firstFrame) * mSize.x) / song->getTotalFrames();
    float valueScale = mOverviewHeight / 2.f / song->getMaxPowerValue();
    drawOverviewWave (song, firstFrame, playFrame, playFrameX, valueScale, mono);

    if (mOverviewPressed) {
      //{{{  animate on
      if (mOverviewLens < mSize.y / 16.f) {
        mOverviewLens += mSize.y / 16.f / 6.f;
        //mWindow->changed();
        }
      }
      //}}}
    else {
      //{{{  animate off
      if (mOverviewLens > 1.f) {
        mOverviewLens /= 2.f;
        //mWindow->changed();
        }
      else if (mOverviewLens > 0.f) {
        // finish animate
        mOverviewLens = 0.f;
        //mWindow->changed();
        }
      }
      //}}}

    if (mOverviewLens > 0.f) {
      float overviewLensCentreX = (float)playFrameX;
      if (overviewLensCentreX - mOverviewLens < 0.f)
        overviewLensCentreX = (float)mOverviewLens;
      else if (overviewLensCentreX + mOverviewLens > mSize.x)
        overviewLensCentreX = mSize.x - mOverviewLens;

      drawOverviewLens (song, playFrame, overviewLensCentreX, mOverviewLens-1.f, mono);
      }

    else {
      //  draw playFrame
      auto framePtr = song->findFrameByFrameNum (playFrame);
      if (framePtr && framePtr->getPowerValues()) {
        //vg->beginPath();
        auto powerValues = framePtr->getPowerValues();
        float yorg = mono ? (mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f)) :
                            (mDstOverviewCentre - (powerValues[0] * valueScale));
        float ylen = mono ? (powerValues[0] * valueScale * 2.f) : ((powerValues[0] + powerValues[1]) * valueScale);
        //vg->rect (cPointF(playFrameX, yorg), cPointF(1.f, ylen));
        //vg->setFillColour (kWhiteF);
        //vg->triangleFill();
        }
      }
    }
  //}}}
  //{{{
  void drawContents (cApp& app) {

    mSize = ImGui::GetWindowSize();
    //{{{  draw top line buttons
    // monoSpaced buttom
    if (toggleButton ("mono",  mShowMonoSpaced))
      toggleShowMonoSpaced();

    // vsync button,fps
    if (app.getPlatform().hasVsync()) {
      // vsync button
      ImGui::SameLine();
      if (toggleButton ("vSync", app.getPlatform().getVsync()))
        app.getPlatform().toggleVsync();

      // fps text
      ImGui::SameLine();
      ImGui::Text (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      }

    // fullScreen button
    if (app.getPlatform().hasFullScreen()) {
      ImGui::SameLine();
      if (toggleButton ("full", app.getPlatform().getFullScreen()))
        app.getPlatform().toggleFullScreen();
      }

    // vertice debug
    ImGui::SameLine();
    ImGui::Text (fmt::format ("{}:{}",
                 ImGui::GetIO().MetricsRenderVertices, ImGui::GetIO().MetricsRenderIndices/3).c_str());
    //}}}
    //{{{  radio buttons
    ImGui::Begin (getName().c_str(), NULL, ImGuiWindowFlags_NoDocking);
    if (ImGui::Button ("radio1"))
      mSongLoader.launchLoad (kRadio1);
    if (ImGui::Button ("radio2"))
      mSongLoader.launchLoad (kRadio2);
    if (ImGui::Button ("radio3"))
      mSongLoader.launchLoad (kRadio3);
    if (ImGui::Button ("radio4"))
      mSongLoader.launchLoad (kRadio4);
    if (ImGui::Button ("radio5"))
      mSongLoader.launchLoad (kRadio5);
    if (ImGui::Button ("radio6"))
      mSongLoader.launchLoad (kRadio6);
    if (ImGui::Button ("wqxr"))
      mSongLoader.launchLoad (kWqxr);
    ImGui::End();
    //}}}

    if (isDrawMonoSpaced())
      ImGui::PushFont (app.getMonoFont());
    mDrawContext.update (24.f, mShowMonoSpaced);

    //{{{  layout
    mWaveHeight = 100.f;
    mOverviewHeight = mShowOverview ? 100.f : 0.f;
    mRangeHeight = 8.f;
    mFreqHeight = mSize.y - mRangeHeight - mWaveHeight - mOverviewHeight;

    mDstFreqTop = 0.f;
    mDstWaveTop = mDstFreqTop + mFreqHeight;
    mDstRangeTop = mDstWaveTop + mWaveHeight;
    mDstOverviewTop = mDstRangeTop + mRangeHeight;

    mDstWaveCentre = mDstWaveTop + (mWaveHeight/2.f);
    mDstOverviewCentre = mDstOverviewTop + (mOverviewHeight/2.f);
    //}}}
    cSong* song = mSongLoader.getSong();

    if (song) {
      { // locked scope
      shared_lock<shared_mutex> lock (song->getSharedMutex());

      // wave left right frames, clip right not left
      int64_t playFrame = song->getPlayFrameNum();
      int64_t leftWaveFrame = playFrame - (((int(mSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
      int64_t rightWaveFrame = playFrame + (((int(mSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
      rightWaveFrame = min (rightWaveFrame, song->getLastFrameNum());

      drawRange (song, playFrame, leftWaveFrame, rightWaveFrame);
      if (song->getNumFrames()) {
        bool mono = song->getNumChannels() == 1;
        drawWave (song, playFrame, leftWaveFrame, rightWaveFrame, mono);
        if (mShowOverview)
          drawOverview (song, playFrame, mono);
        drawFreq (song, playFrame);
        }
      }
      //{{{  draw playTime big centre
      mDrawContext.text ({mSize.x/2.f, mSize.y - mDrawContext.mLineHeight}, eText, song->getPlayTimeString (0));
      //}}}
      //{{{  draw firstTime, left
      mDrawContext.smallText ({0.f, mSize.y - mDrawContext.mLineHeight}, eText, song->getFirstTimeString (0));
      //}}}
      //{{{  draw lastTime, right
      float width = mDrawContext.measureSmallText (song->getLastTimeString (0));
      mDrawContext.smallText ({mSize.x - width, mSize.y - mDrawContext.mLineHeight}, eText, song->getLastTimeString (0));
      //}}}
      }

    if (isDrawMonoSpaced())
      ImGui::PopFont();
    }
  //}}}

  //{{{
  class cDrawContext {
  public:
    //{{{
    void update (float fontSize, bool monoSpaced) {
    // update draw context

      mDrawList = ImGui::GetWindowDrawList();

      mFont = ImGui::GetFont();
      mMonoSpaced = monoSpaced;

      mFontAtlasSize = ImGui::GetFontSize();
      mFontSize = fontSize;
      mFontSmallSize = mFontSize > ((mFontAtlasSize * 2.f) / 3.f) ? ((mFontAtlasSize * 2.f) / 3.f) : mFontSize;
      mFontSmallOffset = ((mFontSize - mFontSmallSize) * 2.f) / 3.f;

      float scale = mFontSize / mFontAtlasSize;
      mLineHeight = ImGui::GetTextLineHeight() * scale;
      mGlyphWidth = measureSpace();
      }
    //}}}

    //{{{
    float measureSpace() const {
    // return width of space

      char str[2] = {' ', 0};
      return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, str+1).x;
      }
    //}}}
    //{{{
    float measureChar (char ch) const {
    // return width of text

      if (mMonoSpaced)
        return mGlyphWidth;
      else {
        char str[2] = {ch,0};
        return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, str+1).x;
        }
      }
    //}}}
    //{{{
    float measureText (const char* str, const char* strEnd) const {
      if (mMonoSpaced)
        return mGlyphWidth * static_cast<uint32_t>(strEnd - str);
      else
        return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
      }
    //}}}
    //{{{
    float measureText (const string& text) const {
      if (mMonoSpaced)
        return mGlyphWidth * static_cast<uint32_t>(text.size());
      else
        return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, text.c_str()).x;
      }
    //}}}
    //{{{
    float measureSmallText (const string& text) const {
      if (mMonoSpaced)
        return mGlyphWidth * mFontSmallSize * static_cast<uint32_t>(text.size());
      else
        return mFont->CalcTextSizeA (mFontSmallSize, FLT_MAX, -1.f, text.c_str()).x;
      }
    //}}}

    //{{{
    float text (ImVec2 pos, uint8_t color, const string& text) {
     // draw and return width of text

      mDrawList->AddText (mFont, mFontSize, pos, kPalette[color], text.c_str(), nullptr);

      if (mMonoSpaced)
        return text.size() * mGlyphWidth;
      else
        return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, text.c_str(), nullptr).x;
      }
    //}}}
    //{{{
    float text (ImVec2 pos, uint8_t color, const char* str, const char* strEnd) {
     // draw and return width of text

      mDrawList->AddText (mFont, mFontSize, pos, kPalette[color], str, strEnd);

      if (mMonoSpaced)
        return measureText (str, strEnd);
      else
        return mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
      }
    //}}}
    //{{{
    float smallText (ImVec2 pos, uint8_t color, const string& text) {
     // draw and return width of small text

      pos.y += mFontSmallOffset;
      mDrawList->AddText (mFont, mFontSmallSize, pos, kPalette[color], text.c_str(), nullptr);

      return mFont->CalcTextSizeA (mFontSmallSize, FLT_MAX, -1.f, text.c_str(), nullptr).x;
      }
    //}}}

    //{{{
    void line (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
      mDrawList->AddLine (pos1, pos2, kPalette[color]);
      }
    //}}}
    //{{{
    void circle (ImVec2 centre, float radius, uint8_t color) {
      mDrawList->AddCircleFilled (centre, radius, kPalette[color], 4);
      }
    //}}}
    //{{{
    void rect (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
      mDrawList->AddRectFilled (pos1, pos2, kPalette[color]);
      }
    //}}}
    //{{{
    void rectLine (ImVec2 pos1, ImVec2 pos2, uint8_t color) {
      mDrawList->AddRect (pos1, pos2, kPalette[color], 1.f);
      }
    //}}}

    float mFontSize = 0.f;
    float mGlyphWidth = 0.f;
    float mLineHeight = 0.f;

  private:
    ImDrawList* mDrawList = nullptr;

    ImFont* mFont = nullptr;
    bool mMonoSpaced = false;

    float mFontAtlasSize = 0.f;
    float mFontSmallSize = 0.f;
    float mFontSmallOffset = 0.f;
    };
  //}}}

  //{{{  vars
  bool mFileLoaded = false;

  bool mShowMonoSpaced = false;
  cDrawContext mDrawContext;

  cSongLoader mSongLoader;

  ImVec2 mSize = {0.f,0.f};
  int64_t mImagePts = 0;
  int mImageId = -1;

  bool mShowGraphics = true;

  float mMove = 0;
  bool mMoved = false;
  float mPressInc = 0;

  // zoom - 0 unity, > 0 zoomOut framesPerPix, < 0 zoomIn pixPerFrame
  int mZoom = 0;
  int mMinZoom = -8;
  int mMaxZoom = 8;
  int mFrameWidth = 1;
  int mFrameStep = 1;

  double mPressedFrameNum = 0;
  bool mOverviewPressed = false;
  bool mRangePressed = false;

  bool mShowOverview = false;
  float mOverviewLens = 0.f;

  // vertical layout
  float mFreqHeight = 0.f;
  float mWaveHeight = 0.f;
  float mRangeHeight = 0.f;
  float mOverviewHeight = 0.f;

  float mDstFreqTop = 0.f;
  float mDstWaveTop = 0.f;
  float mDstRangeTop = 0.f;
  float mDstOverviewTop = 0.f;

  float mDstWaveCentre = 0.f;
  float mDstOverviewCentre = 0.f;

  // mOverview cache
  int64_t mOverviewTotalFrames = 0;
  int64_t mOverviewLastFrame = 0;
  int64_t mOverviewFirstFrame = 0;
  float mOverviewValueScale = 0.f;

  float mOverviewValuesL [1920] = { 0.f };
  float mOverviewValuesR [1920] = { 0.f };
  //}}}
  //{{{
  static cUI* create (const string& className) {
    return new cPlayerUI (className);
    }
  //}}}
  inline static const bool mRegistered = registerClass ("player", &create);
  };

//cDrawSong.h
#pragma once
//{{{  includes
#include <cstdint>
#include <array>
#include <string>

// imgui
#include "../imgui/imgui.h"
#include "../imgui/myImguiWidgets.h"

// ui
#include "../ui/cUI.h"

// song
#include "../song/cSongLoader.h"
#include "../song/cSong.h"

// utils
#include "../utils/cLog.h"
#include "../utils/date.h"

using namespace std;
//}}}

//{{{
class cDrawContext {
public:
  //{{{  palette const
  static const uint8_t eBackground =  0;
  static const uint8_t eText =        1;
  static const uint8_t eFreq =        2;

  static const uint8_t ePeak =        3;
  static const uint8_t ePowerPlayed = 4;
  static const uint8_t ePowerPlay =   5;
  static const uint8_t ePowerToPlay = 6;

  static const uint8_t eRange       = 7;
  static const uint8_t eOverview    = 8;

  static const uint8_t eLensOutline = 9;
  static const uint8_t eLensPower  = 10;
  static const uint8_t eLensPlay   = 11;
  //}}}

  float getFontSize() const { return mFontSize; }
  float getGlyphWidth() const { return mGlyphWidth; }
  float getLineHeight() const { return mLineHeight; }

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

  // measure text
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
  float measureText (const string& text) const {
    return mMonoSpaced ? mGlyphWidth * static_cast<uint32_t>(text.size())
                       : mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, text.c_str()).x;
    }
  //}}}
  //{{{
  float measureText (const char* str, const char* strEnd) const {
    return mMonoSpaced ? mGlyphWidth * static_cast<uint32_t>(strEnd - str)
                       : mFont->CalcTextSizeA (mFontSize, FLT_MAX, -1.f, str, strEnd).x;
    }
  //}}}
  //{{{
  float measureSmallText (const string& text) const {
    return mMonoSpaced ? mGlyphWidth * (mFontSmallSize / mFontSize) * static_cast<uint32_t>(text.size())
                       : mFont->CalcTextSizeA (mFontSmallSize, FLT_MAX, -1.f, text.c_str()).x;
    }
  //}}}

  // text
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

  // graphics
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

private:
  //{{{
  inline static const array <ImU32,12> kPalette = {
  //  aabbggrr
    0xff000000, // eBackground
    0xffffffff, // eText
    0xff00ffff, // eFreq
    0xff606060, // ePeak
    0xffc00000, // ePowerPlayed
    0xffffffff, // ePowerPlay
    0xff808080, // ePowerToPlay
    0xff800080, // eRange
    0xffa0a0a0, // eOverView
    0xffa000a0, // eLensOutline
    0xffa040a0, // eLensPower
    0xffa04060, // eLensPlay
    };
  //}}}

  ImDrawList* mDrawList = nullptr;

  ImFont* mFont = nullptr;
  bool mMonoSpaced = false;

  float mFontAtlasSize = 0.f;
  float mFontSmallSize = 0.f;
  float mFontSmallOffset = 0.f;

  float mFontSize = 0.f;
  float mGlyphWidth = 0.f;
  float mLineHeight = 0.f;
  };
//}}}

class cDrawSong : public cDrawContext {
public:
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

  //{{{
  void draw (cSong* song, bool monoSpaced) {

    mSong = song;
    if (!song)
      return;

    update (24.f, monoSpaced);
    layout();

    { // locked scope
    shared_lock<shared_mutex> lock (mSong->getSharedMutex());

    // wave left right frames, clip right not left
    int64_t playFrame = mSong->getPlayFrameNum();
    int64_t leftWaveFrame = playFrame - (((int(mSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
    int64_t rightWaveFrame = playFrame + (((int(mSize.x)+mFrameWidth)/2) * mFrameStep) / mFrameWidth;
    rightWaveFrame = min (rightWaveFrame, mSong->getLastFrameNum());

    if (mSong->getNumFrames()) {
      bool mono = (mSong->getNumChannels() == 1);
      drawWave (playFrame, leftWaveFrame, rightWaveFrame, mono);
      if (mShowOverview)
        drawOverview (playFrame, mono);
      drawFreq (playFrame);
      }
    drawRange (playFrame, leftWaveFrame, rightWaveFrame);
    }

    // draw firstTime left
    smallText ({0.f, mSize.y - getLineHeight()}, eText, mSong->getFirstTimeString(0));

    // draw playTime entre
    text ({mSize.x/2.f, mSize.y - getLineHeight()}, eText, mSong->getPlayTimeString(0));

    // draw lastTime right
    float width = measureSmallText (mSong->getLastTimeString (0));
    smallText ({mSize.x - width, mSize.y - getLineHeight()}, eText, mSong->getLastTimeString(0));
    }
  //}}}

private:
  //{{{
  void layout () {

    ImVec2 size = ImGui::GetWindowSize();
    mChanged |= (size.x != mSize.x) || (size.y != mSize.y);
    mSize = size;

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
    }
  //}}}

  //{{{
  void drawWave (int64_t playFrame, int64_t leftFrame, int64_t rightFrame, bool mono) {

    (void)mono;
    array <float,2> values = { 0.f };

    float peakValueScale = mWaveHeight / mSong->getMaxPeakValue() / 2.f;
    float powerValueScale = mWaveHeight / mSong->getMaxPowerValue() / 2.f;

    float xlen = (float)mFrameStep;
    if (mFrameStep == 1) {
      //{{{  draw all peak values
      float xorg = 0;
      for (int64_t frame = leftFrame; frame < rightFrame; frame += mFrameStep) {
        cSong::cFrame* framePtr = mSong->findFrameByFrameNum (frame);
        if (framePtr) {
          // draw frame peak values scaled to maxPeak
          if (framePtr->getPowerValues()) {
            float* peakValuesPtr = framePtr->getPeakValues();
            for (size_t i = 0; i < 2; i++)
              values[i] = *peakValuesPtr++ * peakValueScale;
            }
          rect ({xorg, mDstWaveCentre - values[0]}, {xorg + xlen, mDstWaveCentre + values[1]}, ePeak);
          }
        xorg += xlen;
        }
      }
      //}}}

    float xorg = 0;
    //{{{  draw powerValues before playFrame, summed if zoomed out
    for (int64_t frame = leftFrame; frame < playFrame; frame += mFrameStep) {
      cSong::cFrame* framePtr = mSong->findFrameByFrameNum (frame);
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
            cSong::cFrame* sumFramePtr = mSong->findFrameByFrameNum (sumFrame);
            if (sumFramePtr) {
              if (sumFramePtr->getPowerValues()) {
                float* powerValuesPtr = sumFramePtr->getPowerValues();
                for (size_t i = 0; i < 2; i++)
                  values[i] += *powerValuesPtr++ * powerValueScale;
                }
              }
            }

          for (size_t i = 0; i < 2; i++)
            values[i] /= toSumFrame - alignedFrame + 1;
          }

        rect ({xorg, mDstWaveCentre - values[0]},
                           {xorg + xlen, mDstWaveCentre + values[1]}, ePowerPlayed);
        }

      xorg += xlen;
      }
    //}}}
    //{{{  draw powerValues playFrame, no sum
    // power scaled to maxPeak
    cSong::cFrame* framePtr = mSong->findFrameByFrameNum (playFrame);
    if (framePtr) {
      //  draw play frame power scaled to maxPeak
      if (framePtr->getPowerValues()) {
        float* powerValuesPtr = framePtr->getPowerValues();
        for (size_t i = 0; i < 2; i++)
          values[i] = *powerValuesPtr++ * peakValueScale;
        }

      rect ({xorg, mDstWaveCentre - values[0]},
                         {xorg + xlen, mDstWaveCentre + values[1]}, ePowerPlay);
      }

    xorg += xlen;
    //}}}
    //{{{  draw powerValues after playFrame, summed if zoomed out
    for (int64_t frame = playFrame+mFrameStep; frame < rightFrame; frame += mFrameStep) {
      cSong::cFrame* sumFramePtr = mSong->findFrameByFrameNum (frame);
      if (sumFramePtr) {
        if (mFrameStep == 1) {
          // power scaled to maxPeak
          if (sumFramePtr->getPowerValues()) {
            float* powerValuesPtr = sumFramePtr->getPowerValues();
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
            cSong::cFrame* toSumFramePtr = mSong->findFrameByFrameNum (sumFrame);
            if (toSumFramePtr) {
              if (toSumFramePtr->getPowerValues()) {
                float* powerValuesPtr = toSumFramePtr->getPowerValues();
                for (size_t i = 0; i < 2; i++)
                  values[i] += *powerValuesPtr++ * powerValueScale;
                }
              }
            }

          for (size_t i = 0; i < 2; i++)
            values[i] /= toSumFrame - alignedFrame + 1;
          }

        rect ({xorg, mDstWaveCentre - values[0]},
                           {xorg + xlen, mDstWaveCentre + values[1]}, ePowerToPlay);
        }

      xorg += xlen;
      }
    //}}}

    //{{{  copy reversed spectrum column to bitmap, clip high freqs to height
    //int freqSize = min (mSong->getNumFreqBytes(), (int)mFreqHeight);
    //int freqOffset = mSong->getNumFreqBytes() > (int)mFreqHeight ? mSong->getNumFreqBytes() - (int)mFreqHeight : 0;

    // bitmap sampled aligned to mFrameStep, !!! could sum !!! ?? ok if neg frame ???
    //auto alignedFromFrame = fromFrame - (fromFrame % mFrameStep);
    //for (auto frame = alignedFromFrame; frame < toFrame; frame += mFrameStep) {
      //auto framePtr = mSong->getAudioFramePtr (frame);
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
  void drawFreq (int64_t playFrame) {

    float valueScale = 100.f / 255.f;

    float xorg = 0.f;
    cSong::cFrame* framePtr = mSong->findFrameByFrameNum (playFrame);
    if (framePtr && framePtr->getFreqValues()) {
      uint8_t* freqValues = framePtr->getFreqValues();
      for (size_t i = 0; (i < mSong->getNumFreqBytes()) && ((i*2) < int(mSize.x)); i++) {
        float value =  freqValues[i] * valueScale;
        if (value > 1.f)
          rect ({xorg, mSize.y - value}, {xorg + 2.f, 0 + mSize.y}, eFreq);
        xorg += 2.f;
        }
      }
    }
  //}}}
  //{{{
  void drawOverviewWave (int64_t firstFrame, int64_t playFrame, float playFrameX, float valueScale, bool mono) {
  // simple overview cache, invalidate if anything changed

    (void)playFrame;
    (void)playFrameX;
    int64_t lastFrame = mSong->getLastFrameNum();
    int64_t totalFrames = mSong->getTotalFrames();

    bool changed = mChanged ||
                   (mOverviewTotalFrames != totalFrames) ||
                   (mOverviewFirstFrame != firstFrame) ||
                   (mOverviewLastFrame != lastFrame) ||
                   (mOverviewValueScale != valueScale);

    array <float,2> values = { 0.f };

    float xorg = 0.f;
    float xlen = 1.f;
    for (size_t x = 0; x < int(mSize.x); x++) {
      // iterate widget width
      if (changed) {
        int64_t frame = firstFrame + ((x * totalFrames) / int(mSize.x));
        int64_t toFrame = firstFrame + (((x+1) * totalFrames) / int(mSize.x));
        if (toFrame > lastFrame)
          toFrame = lastFrame+1;

        cSong::cFrame* framePtr = mSong->findFrameByFrameNum (frame);
        if (framePtr && framePtr->getPowerValues()) {
          // accumulate frame, handle silence better
          float* powerValues = framePtr->getPowerValues();
          values[0] = powerValues[0];
          values[1] = mono ? 0 : powerValues[1];

          if (frame < toFrame) {
            int numSummedFrames = 1;
            frame++;
            while (frame < toFrame) {
              framePtr = mSong->findFrameByFrameNum (frame);
              if (framePtr) {
                if (framePtr->getPowerValues()) {
                  float* sumPowerValues = framePtr->getPowerValues();
                  values[0] += sumPowerValues[0];
                  values[1] += mono ? 0 : sumPowerValues[1];
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

      rect ({xorg, mDstOverviewCentre - mOverviewValuesL[x]},
            {xorg + xlen, mDstOverviewCentre - mOverviewValuesL[x] + mOverviewValuesR[x]}, eOverview);
      xorg += 1.f;
      }

    // possible cache to stop recalc
    mOverviewTotalFrames = totalFrames;
    mOverviewLastFrame = lastFrame;
    mOverviewFirstFrame = firstFrame;
    mOverviewValueScale = valueScale;
    }
  //}}}
  //{{{
  void drawOverviewLens (int64_t playFrame, float centreX, float width, bool mono) {
  // draw frames centred at playFrame -/+ width in els, centred at centreX

    cLog::log (LOGINFO, "drawOverviewLens %d %f %f", playFrame, centreX, width);

    // cut hole and frame it
    rect ({centreX - width, mDstOverviewTop},
          {centreX - width + (width * 2.f), mDstOverviewTop + mOverviewHeight}, eBackground);
    rectLine ({centreX - width, mDstOverviewTop},
              {centreX - width + (width * 2.f), mDstOverviewTop + mOverviewHeight}, eLensOutline);

    // calc leftmost frame, clip to valid frame, adjust firstX which may overlap left up to mFrameWidth
    float leftFrame = playFrame - width;
    float firstX = centreX - (playFrame - leftFrame);
    if (leftFrame < 0) {
      firstX += -leftFrame;
      leftFrame = 0;
      }

    int64_t rightFrame = playFrame + (int64_t)width;
    rightFrame = min (rightFrame, mSong->getLastFrameNum());

    // calc lens max power
    float maxPowerValue = 0.f;
    for (int64_t frame = int(leftFrame); frame <= rightFrame; frame++) {
      cSong::cFrame*framePtr = mSong->findFrameByFrameNum (frame);
      if (framePtr && framePtr->getPowerValues()) {
        float* powerValues = framePtr->getPowerValues();
        maxPowerValue = max (maxPowerValue, powerValues[0]);
        if (!mono)
          maxPowerValue = max (maxPowerValue, powerValues[1]);
        }
      }

    // draw unzoomed waveform, start before playFrame
    float xorg = firstX;
    float valueScale = mOverviewHeight / 2.f / maxPowerValue;
    for (int64_t frame = int(leftFrame); frame <= rightFrame; frame++) {
      cSong::cFrame* framePtr = mSong->findFrameByFrameNum (frame);
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

        float* powerValues = framePtr->getPowerValues();
        float yorg = mono ? mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f) :
                            mDstOverviewCentre - (powerValues[0] * valueScale);
        float ylen = mono ? powerValues[0] * valueScale * 2.f  :
                            (powerValues[0] + powerValues[1]) * valueScale;
        rect ({xorg, yorg}, {xorg + 1.f, yorg + ylen}, eLensPower);

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
    }
  //}}}
  //{{{
  void drawOverview (int64_t playFrame, bool mono) {

    if (!mSong->getTotalFrames())
      return;

    int64_t firstFrame = mSong->getFirstFrameNum();
    float playFrameX = ((playFrame - firstFrame) * mSize.x) / mSong->getTotalFrames();
    float valueScale = mOverviewHeight / 2.f / mSong->getMaxPowerValue();
    drawOverviewWave (firstFrame, playFrame, playFrameX, valueScale, mono);

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

      drawOverviewLens (playFrame, overviewLensCentreX, mOverviewLens-1.f, mono);
      }

    else {
      //  draw playFrame
      cSong::cFrame* framePtr = mSong->findFrameByFrameNum (playFrame);
      if (framePtr && framePtr->getPowerValues()) {
        float* powerValues = framePtr->getPowerValues();
        float yorg = mono ? (mDstOverviewTop + mOverviewHeight - (powerValues[0] * valueScale * 2.f)) :
                            (mDstOverviewCentre - (powerValues[0] * valueScale));
        float ylen = mono ? (powerValues[0] * valueScale * 2.f) : ((powerValues[0] + powerValues[1]) * valueScale);
        rect ({playFrameX, yorg}, {playFrameX + 1.f, yorg + ylen}, eLensPlay);
        }
      }
    }
  //}}}
  //{{{
  void drawRange (int64_t playFrame, int64_t leftFrame, int64_t rightFrame) {

    (void)playFrame;
    (void)leftFrame;
    (void)rightFrame;

    rect ({0.f, mDstRangeTop}, {mSize.x, mDstRangeTop + mRangeHeight}, eRange);

    //for (auto &item : mSong->getSelect().getItems()) {
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
    }
  //}}}

  //{{{  vars
  cSong* mSong = nullptr;

  bool mShowOverview = true;

  bool mChanged = false;
  ImVec2 mSize = {0.f,0.f};

  int64_t mImagePts = 0;
  int mImageId = -1;

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
  };

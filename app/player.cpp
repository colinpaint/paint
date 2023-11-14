// player.cpp - imgui player main,app,UI
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>

// stb - invoke header only library implementation here
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

// utils
#include "../common/date.h"
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"
#include "fmt/format.h"

// song
#include "../song/cSong.h"
#include "../song/cSongLoader.h"

// app
#include "cApp.h"
#include "cPlatform.h"
#include "cGraphics.h"
#include "myImgui.h"
#include "cUI.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

#include "../common/cFileView.h"

using namespace std;
//}}}
namespace {
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
  }

//{{{
class cPlayerApp : public cApp {
public:
  cPlayerApp (const cPoint& windowSize, bool fullScreen, bool vsync);
  virtual ~cPlayerApp() = default;

  string getSongName() const { return mSongName; }
  cSong* getSong() const;

  bool setSongName (const string& songName);
  bool setSongSpec (const vector <string>& songSpec);

  virtual void drop (const vector<string>& dropItems) final;

private:
  cSongLoader* mSongLoader;
  string mSongName;
  };

cPlayerApp::cPlayerApp (const cPoint& windowSize, bool fullScreen, bool vsync)
    : cApp("player",windowSize, fullScreen, vsync) {
  mSongLoader = new cSongLoader();
  }

cSong* cPlayerApp::getSong() const {
  return mSongLoader->getSong();
  }

bool cPlayerApp::setSongName (const string& songName) {

  mSongName = songName;

  // load song
  const vector <string>& strings = { songName };
  mSongLoader->launchLoad (strings);

  return true;
  }

bool cPlayerApp::setSongSpec (const vector <string>& songSpec) {

  mSongName = songSpec[0];
  mSongLoader->launchLoad (songSpec);
  return true;
  }

void cPlayerApp::drop (const vector<string>& dropItems) {

  for (auto& item : dropItems) {
    cLog::log (LOGINFO, item);
    setSongName (cFileUtils::resolve (item));
    }
  }
//}}}
//{{{
class cDrawSong : public cDrawContext {
public:
  cDrawSong() : cDrawContext (kPalette) {}
  //{{{
  //void onDown (cPointF point) {

    //cWidget::onDown (point);

    //cSong* song = mLoader->getSong();

    //if (song) {
      ////shared_lock<shared_mutex> lock (song->getSharedMutex());
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
      ////shared_lock<shared_mutex> lock (song.getSharedMutex());
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

    //mZoom = min (max (zoom, mMinZoom), mMaxZoom);

    //// zoomIn expanding frame to mFrameWidth pix
    //mFrameWidth = (mZoom < 0) ? -mZoom+1 : 1;

    //// zoomOut summing mFrameStep frames per pix
    //mFrameStep = (mZoom > 0) ? mZoom+1 : 1;
    //}
  //}}}
  //{{{
  void draw (cSong* song, bool monoSpaced) {

    if (!song)
      return;
    mSong = song;

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
    smallText ({0.f, mSize.y - getLineHeight()}, eText, mSong->getFirstTimeString (0));

    // draw playTime entre
    float width = measureSmallText (mSong->getPlayTimeString (0));
    text ({mSize.x/2.f - width/2.f, mSize.y - getLineHeight()}, eText, mSong->getPlayTimeString (0));

    // draw lastTime right
    width = measureSmallText (mSong->getLastTimeString (0));
    smallText ({mSize.x - width, mSize.y - getLineHeight()}, eText, mSong->getLastTimeString (0));
    }
  //}}}

private:
  //{{{  palette const
  inline static const uint8_t eBackground =  0;
  inline static const uint8_t eText =        1;
  inline static const uint8_t eFreq =        2;

  inline static const uint8_t ePeak =        3;
  inline static const uint8_t ePowerPlayed = 4;
  inline static const uint8_t ePowerPlay =   5;
  inline static const uint8_t ePowerToPlay = 6;

  inline static const uint8_t eRange       = 7;
  inline static const uint8_t eOverview    = 8;

  inline static const uint8_t eLensOutline = 9;
  inline static const uint8_t eLensPower  = 10;
  inline static const uint8_t eLensPlay   = 11;

  inline static const vector <ImU32> kPalette = {
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

  //{{{
  void layout () {

    // check for window size change, refresh any caches dependent on size
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
      for (int64_t frame = leftFrame; frame < rightFrame; frame += mFrameStep, xorg += xlen) {
        cSong::cFrame* framePtr = mSong->findFrameByFrameNum (frame);
        if (framePtr && framePtr->getPowerValues()) {
          // draw frame peak values scaled to maxPeak
          float* peakValuesPtr = framePtr->getPeakValues();
          values[0] = *peakValuesPtr * peakValueScale;
          values[1] = *(peakValuesPtr+1) * peakValueScale;
          rect ({xorg, mDstWaveCentre - values[0]}, {xorg + xlen, mDstWaveCentre + values[1]}, ePeak);
          }
        }
      }
      //}}}

    float xorg = 0;
    //{{{  draw powerValues before playFrame, summed if zoomed out
    for (int64_t frame = leftFrame; frame < playFrame; frame += mFrameStep, xorg += xlen) {
      cSong::cFrame* framePtr = mSong->findFrameByFrameNum (frame);
      if (framePtr) {
        if (mFrameStep == 1) {
          // power scaled to maxPeak
          if (framePtr->getPowerValues()) {
           float* powerValuesPtr = framePtr->getPowerValues();
            values[0] = powerValuesPtr[0] * peakValueScale;
            values[1] = powerValuesPtr[1] * peakValueScale;
            }
          }
        else {
          // sum mFrameStep frames, mFrameStep aligned, scaled to maxPower
          values[0] = 0.f;
          values[1] = 0.f;

          int64_t alignedFrame = frame - (frame % mFrameStep);
          int64_t toSumFrame = min (alignedFrame + mFrameStep, rightFrame);
          for (int64_t sumFrame = alignedFrame; sumFrame < toSumFrame; sumFrame++) {
            cSong::cFrame* sumFramePtr = mSong->findFrameByFrameNum (sumFrame);
            if (sumFramePtr && sumFramePtr->getPowerValues()) {
              float* powerValuesPtr = sumFramePtr->getPowerValues();
              values[0] += powerValuesPtr[0] * powerValueScale;
              values[1] += powerValuesPtr[1] * powerValueScale;
              }
            }

          values[0] /= toSumFrame - alignedFrame + 1;
          values[1] /= toSumFrame - alignedFrame + 1;
          }

        rect ({xorg, mDstWaveCentre - values[0]}, {xorg + xlen, mDstWaveCentre + values[1]}, ePowerPlayed);
        }
      }
    //}}}
    //{{{  draw powerValues playFrame, no sum
    // power scaled to maxPeak
    cSong::cFrame* framePtr = mSong->findFrameByFrameNum (playFrame);
    if (framePtr && framePtr->getPowerValues()) {
      // draw play frame power scaled to maxPeak
      float* powerValuesPtr = framePtr->getPowerValues();
      values[0] = powerValuesPtr[0] * peakValueScale;
      values[1] = powerValuesPtr[1] * peakValueScale;

      rect ({xorg, mDstWaveCentre - values[0]}, {xorg + xlen, mDstWaveCentre + values[1]}, ePowerPlay);
      }

    xorg += xlen;
    //}}}
    //{{{  draw powerValues after playFrame, summed if zoomed out
    for (int64_t frame = playFrame + mFrameStep; frame < rightFrame; frame += mFrameStep, xorg += xlen) {
      cSong::cFrame* sumFramePtr = mSong->findFrameByFrameNum (frame);
      if (sumFramePtr && sumFramePtr->getPowerValues()) {
        if (mFrameStep == 1) {
          // power scaled to maxPeak
          float* powerValuesPtr = sumFramePtr->getPowerValues();
          values[0] = powerValuesPtr[0] * peakValueScale;
          values[1] = powerValuesPtr[1] * peakValueScale;
          }
        else {
          // sum mFrameStep frames, mFrameStep aligned, scaled to maxPower
          values[0] = 0.f;
          values[1] = 0.f;

          int64_t alignedFrame = frame - (frame % mFrameStep);
          int64_t toSumFrame = min (alignedFrame + mFrameStep, rightFrame);
          for (int64_t sumFrame = alignedFrame; sumFrame < toSumFrame; sumFrame++) {
            cSong::cFrame* toSumFramePtr = mSong->findFrameByFrameNum (sumFrame);
            if (toSumFramePtr && toSumFramePtr->getPowerValues()) {
              float* powerValuesPtr = toSumFramePtr->getPowerValues();
              values[0] += powerValuesPtr[0] * powerValueScale;
              values[1] += powerValuesPtr[1] * powerValueScale;
              }
            }

          values[0] /= toSumFrame - alignedFrame + 1;
          values[1] /= toSumFrame - alignedFrame + 1;
          }

        rect ({xorg, mDstWaveCentre - values[0]}, {xorg + xlen, mDstWaveCentre + values[1]}, ePowerToPlay);
        }
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
  void drawOverviewWave (int64_t firstFrame, int64_t playFrame, float playFrameX, float valueScale, bool mono) {
  // use simple overview cache, invalidate if anything changed

    (void)playFrame;
    (void)playFrameX;
    int64_t lastFrame = mSong->getLastFrameNum();
    int64_t totalFrames = mSong->getTotalFrames();

    bool changed = mChanged ||
                   (mOverviewTotalFrames != totalFrames) ||
                   (mOverviewFirstFrame != firstFrame) ||
                   (mOverviewLastFrame != lastFrame) ||
                   (mOverviewValueScale != valueScale);

    mOverviewTotalFrames = totalFrames;
    mOverviewLastFrame = lastFrame;
    mOverviewFirstFrame = firstFrame;
    mOverviewValueScale = valueScale;

    float xorg = 0.f;
    float xlen = 1.f;

    if (!changed) {
      //{{{  draw overview cache, return
      for (uint32_t x = 0; x < static_cast<uint32_t>(mSize.x); x++, xorg += 1.f)
        rect ({xorg, mDstOverviewCentre - mOverviewValuesL[x]},
              {xorg + xlen, mDstOverviewCentre - mOverviewValuesL[x] + mOverviewValuesR[x]}, eOverview);
      return;
      }
      //}}}

    array <float,2> values;
    for (uint32_t x = 0; x < static_cast<uint32_t>(mSize.x); x++, xorg += 1.f) {
      // iterate window width
      int64_t frame = firstFrame + ((x * totalFrames) / static_cast<uint32_t>(mSize.x));
      int64_t toFrame = firstFrame + (((x+1) * totalFrames) / static_cast<uint32_t>(mSize.x));
      if (toFrame > lastFrame)
        toFrame = lastFrame+1;

      cSong::cFrame* framePtr = mSong->findFrameByFrameNum (frame);
      if (framePtr && framePtr->getPowerValues()) {
        //{{{  accumulate frame
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
        //}}}

      rect ({xorg, mDstOverviewCentre - mOverviewValuesL[x]},
            {xorg + xlen, mDstOverviewCentre - mOverviewValuesL[x] + mOverviewValuesR[x]}, eOverview);
      }
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

    int64_t rightFrame = playFrame + static_cast<int64_t>(width);
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
  void drawFreq (int64_t playFrame) {

    float valueScale = 100.f / 255.f;

    float xorg = 0.f;
    cSong::cFrame* framePtr = mSong->findFrameByFrameNum (playFrame);
    if (framePtr && framePtr->getFreqValues()) {
      uint8_t* freqValues = framePtr->getFreqValues();
      for (uint32_t i = 0;
           (i < mSong->getNumFreqBytes()) && ((i*2) < static_cast<uint32_t>(mSize.x)); i++, xorg += 2.f)
        rect ({xorg, mSize.y - freqValues[i] * valueScale}, {xorg + 2.f, 0 + mSize.y}, eFreq);
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
        //dc->DrawText (string (title.begin(), title.end()).data(), (uint32_t)title.size(), mWindow->getTextFormat(),
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

  double mPressedFrameNum = 0;
  bool mOverviewPressed = false;
  bool mRangePressed = false;

  // zoom - 0 unity, > 0 zoomOut framesPerPix, < 0 zoomIn pixPerFrame
  int mZoom = 0;
  int mMinZoom = -8;
  int mMaxZoom = 8;
  int mFrameWidth = 1;
  int mFrameStep = 1;

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
  array <float,3000> mOverviewValuesL = { 0.f };
  array <float,3000> mOverviewValuesR = { 0.f };

  int64_t mOverviewTotalFrames = 0;
  int64_t mOverviewLastFrame = 0;
  int64_t mOverviewFirstFrame = 0;
  float mOverviewValueScale = 0.f;
  //}}}
  };
//}}}
//{{{
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

  //{{{
  void addToDrawList (cApp& app) final {

    cPlayerApp& playerApp = (cPlayerApp&)app;

    ImGui::SetNextWindowPos (ImVec2(0,0));
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

    ImGui::Begin ("player", &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

    //{{{  draw top buttons
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
    //{{{  draw radio buttons
    if (ImGui::Button ("radio1"))
      playerApp.setSongSpec (kRadio1);
    if (ImGui::Button ("radio2"))
      playerApp.setSongSpec (kRadio2);
    if (ImGui::Button ("radio3"))
      playerApp.setSongSpec (kRadio3);
    if (ImGui::Button ("radio4"))
      playerApp.setSongSpec (kRadio4);
    if (ImGui::Button ("radio5"))
      playerApp.setSongSpec (kRadio5);
    if (ImGui::Button ("radio6"))
      playerApp.setSongSpec (kRadio6);
    if (ImGui::Button ("wqxr"))
      playerApp.setSongSpec (kWqxr);
    //}}}
    //{{{  draw clockButton
    ImGui::SetCursorPos ({ImGui::GetWindowWidth() - 130.f, 0.f});
    clockButton ("clock", app.getNow(), {110.f,150.f});
    //}}}

    // draw song
    if (isDrawMonoSpaced())
      ImGui::PushFont (app.getMonoFont());

    mDrawSong.draw (playerApp.getSong(), isDrawMonoSpaced());

    if (isDrawMonoSpaced())
      ImGui::PopFont();

    ImGui::End();
    }
  //}}}

private:
  bool isDrawMonoSpaced() { return mShowMonoSpaced; }
  void toggleShowMonoSpaced() { mShowMonoSpaced = ! mShowMonoSpaced; }

  // vars
  bool mOpen = true;
  bool mShowMonoSpaced = false;

  cDrawSong mDrawSong;

  static cUI* create (const string& className) { return new cPlayerUI (className); }
  inline static const bool mRegistered = registerClass ("player", &create);
  };
//}}}

int main (int numArgs, char* args[]) {

  // params
  eLogLevel logLevel = LOGINFO;
  bool fullScreen = false;
  bool vsync = true;
  //{{{  parse command line args to params
  // args to params
  vector <string> params;
  for (int i = 1; i < numArgs; i++)
    params.push_back (args[i]);

  // parse and remove recognised params
  for (auto it = params.begin(); it < params.end();) {
    if (*it == "log1") { logLevel = LOGINFO1; params.erase (it); }
    else if (*it == "log2") { logLevel = LOGINFO2; params.erase (it); }
    else if (*it == "log3") { logLevel = LOGINFO3; params.erase (it); }
    else if (*it == "full") { fullScreen = true; params.erase (it); }
    else if (*it == "free") { vsync = false; params.erase (it); }
    else ++it;
    };
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, fmt::format ("player"));

  // list static registered classes
  cUI::listRegisteredClasses();

  // app
  cPlayerApp app ({800, 480}, fullScreen, vsync);
  app.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 20.f));
  app.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 20.f));
  app.setSongName (params.empty() ? "" : cFileUtils::resolve (params[0]));

  app.mainUILoop();

  return EXIT_SUCCESS;
  }

// player.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define NOMINMAX
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>

#include <sys/stat.h>

// utils
#include "../date/include/date/date.h"
#include "../common/basicTypes.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/myImgui.h"

// decoders
//{{{  include libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  #include "libavutil/motion_vector.h"
  #include "libavutil/frame.h"
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}
#include "../decoders/cAudioFrame.h"
#include "../decoders/cVideoFrame.h"
#include "../decoders/cFFmpegVideoDecoder.h"
#include "../decoders/cSubtitleFrame.h"
#include "../decoders/cSubtitleImage.h"

// dvb
#include "../dvb/cDvbMultiplex.h"
#include "../dvb/cDvbSource.h"
#include "../dvb/cTransportStream.h"
#include "../dvb/cVideoRender.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cPlayer.h"

// song
#include "../song/cSong.h"
#include "../song/cSongLoader.h"

using namespace std;
//}}}

namespace {
  //{{{
  class cPlayerOptions : public cApp::cOptions,
                         public cTransportStream::cOptions,
                         public cRender::cOptions,
                         public cAudioRender::cOptions,
                         public cVideoRender::cOptions,
                         public cFFmpegVideoDecoder::cOptions {
  public:
    virtual ~cPlayerOptions() = default;

    string getString() const { return "sub motion fileName"; }

    // vars
    bool mShowSubtitle = false;
    bool mShowMotionVectors = false;
    string mFileName;
    };
  //}}}

  //{{{  const song channels
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
  //}}}
  //{{{
  class cSongApp : public cApp {
  public:
    //{{{
    cSongApp (cPlayerOptions* options, iUI* ui) : cApp("songApp", options, ui) {
      mSongLoader = new cSongLoader();
      }
    //}}}
    virtual ~cSongApp() = default;

    string getSongName() const { return mSongName; }
    cSong* getSong() const { return mSongLoader->getSong(); }

    //{{{
    bool setSongName (const string& songName) {

      mSongName = cFileUtils::resolve (songName);

      // load song
      const vector <string>& strings = { mSongName };
      mSongLoader->launchLoad (strings);

      return true;
      }
    //}}}
    //{{{
    bool setSongSpec (const vector <string>& songSpec) {
      mSongName = songSpec[0];
      mSongLoader->launchLoad (songSpec);
      return true;
      }
    //}}}

    //{{{
    virtual void drop (const vector<string>& dropItems) final {

      for (auto& item : dropItems) {
        cLog::log (LOGINFO, item);
        setSongName (item);
        }
      }
    //}}}

  private:
    cPlayerOptions* mOptions;
    cSongLoader* mSongLoader;
    string mSongName;
    };
  //}}}
  //{{{
  class cSongUI : public cApp::iUI {
  public:
    virtual void draw (cApp& app) final {

      app.getGraphics().clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                                (int32_t)ImGui::GetIO().DisplaySize.y});

      ImGui::SetNextWindowPos ({0.f,0.f});
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
      ImGui::Begin ("song", nullptr, ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
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

      cSongApp& songApp = (cSongApp&)app;
      //{{{  draw radio buttons
      if (ImGui::Button ("radio1"))
        songApp.setSongSpec (kRadio1);

      if (ImGui::Button ("radio2"))
        songApp.setSongSpec (kRadio2);

      if (ImGui::Button ("radio3"))
        songApp.setSongSpec (kRadio3);

      if (ImGui::Button ("radio4"))
        songApp.setSongSpec (kRadio4);

      if (ImGui::Button ("radio5"))
        songApp.setSongSpec (kRadio5);

      if (ImGui::Button ("radio6"))
        songApp.setSongSpec (kRadio6);

      if (ImGui::Button ("wqxr"))
        songApp.setSongSpec (kWqxr);
      //}}}

      // draw song
      if (isDrawMonoSpaced())
        ImGui::PushFont (app.getMonoFont());
      mDrawSong.draw (songApp.getSong(), isDrawMonoSpaced());
      if (isDrawMonoSpaced())
        ImGui::PopFont();

      //{{{  draw clockButton
      ImGui::SetCursorPos ({ImGui::GetWindowWidth() - 130.f, 0.f});
      clockButton ("clock", app.getNow(), {110.f,110.f});
      //}}}
      ImGui::End();
      }

  private:
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

    bool isDrawMonoSpaced() { return mShowMonoSpaced; }
    void toggleShowMonoSpaced() { mShowMonoSpaced = ! mShowMonoSpaced; }

    // vars
    cDrawSong mDrawSong;
    bool mShowMonoSpaced = false;
    };
  //}}}

  //{{{
  class cPlayerApp : public cApp {
  public:
    cPlayerApp (cPlayerOptions* options, iUI* ui) : cApp ("Player", options, ui), mOptions(options) {}
    virtual ~cPlayerApp() = default;

    cPlayerOptions* getOptions() { return mOptions; }
    bool hasTransportStream() { return mTransportStream != nullptr; }
    cTransportStream& getTransportStream() { return *mTransportStream; }

    // fileSource
    string getFileName() const { return mOptions->mFileName; }
    uint64_t getStreamPos() const { return mStreamPos; }
    uint64_t getFilePos() const { return mFilePos; }
    size_t getFileSize() const { return mFileSize; }
    //{{{
    void addFile (const string& fileName, cPlayerOptions* options) {

      // open fileName
      mFileName = cFileUtils::resolve (fileName);
      FILE* file = fopen (mFileName.c_str(), "rb");
      if (!file) {
        //{{{  error, return
        cLog::log (LOGERROR, fmt::format ("addFile failed to open {}", mOptions->mFileName));
        return;
        }
        //}}}

      // create transportStream
      mTransportStream = new cTransportStream (
        {"file", 0, {}, {}}, options,
        [&](cTransportStream::cService& service) noexcept {
          if (service.getStream (eStreamType(eVideo)).isDefined()) {
            service.enableStream (eVideo);
            service.enableStream (eAudio);
            service.enableStream (eSubtitle);
            }
          },
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo, bool skip) noexcept {
          cLog::log (LOGINFO, fmt::format ("pes {}:{:5d} size:{:6d} {:8d} {} {}",
                                           service.getSid(),
                                           pidInfo.getPid(), pidInfo.getBufSize(),
                                           pidInfo.getStreamPos(),
                                           utils::getFullPtsString (pidInfo.getPts()),
                                           utils::getFullPtsString (pidInfo.getDts())
                                           ));
          cStream* stream = service.getStreamByPid (pidInfo.getPid());
          if (stream && stream->isEnabled())
            if (stream->getRender().decodePes (pidInfo.mBuffer, pidInfo.getBufSize(),
                                               pidInfo.getPts(), pidInfo.getDts(),
                                               pidInfo.mStreamPos, skip))
              // transferred ownership of mBuffer to render, create new one
              pidInfo.mBuffer = (uint8_t*)malloc (pidInfo.mBufSize);
          });
      if (!mTransportStream) {
        //{{{  error, return
        cLog::log (LOGERROR, "addFile cTransportStream create failed");
        return;
        }
        //}}}

      // create fileRead thread
      thread ([=]() {
        cLog::setThreadName ("file");

        mFilePos = 0;
        mStreamPos = 0;
        size_t chunkSize = 188 * 256;
        uint8_t* chunk = new uint8_t[chunkSize];
        while (true) {
          while (mTransportStream->throttle())
            this_thread::sleep_for (1ms);

          // seek and read chunk from file
          bool skip = mStreamPos != mFilePos;
          if (skip) {
            //{{{  seek to mStreamPos
            if (fseek (file, (long)mStreamPos, SEEK_SET))
              cLog::log (LOGERROR, fmt::format ("seek failed {}", mStreamPos));
            else {
              cLog::log (LOGINFO, fmt::format ("seek {}", mStreamPos));
              mFilePos = mStreamPos;
              }
            }
            //}}}
          size_t bytesRead = fread (chunk, 1, chunkSize, file);
          mFilePos = mFilePos + bytesRead;
          if (bytesRead > 0)
            mStreamPos += mTransportStream->demux (chunk, bytesRead, mStreamPos, skip);
          else
            break;

          //{{{  update fileSize
          #ifdef _WIN32
            struct _stati64 st;
            if (_stat64 (mOptions->mFileName.c_str(), &st) != -1)
              mFileSize = st.st_size;
          #else
            struct stat st;
            if (stat (mOptions->mFileName.c_str(), &st) != -1)
              mFileSize = st.st_size;
          #endif
          //}}}
          }

        fclose (file);
        delete[] chunk;

        cLog::log (LOGERROR, "exit");
        }).detach();
      }
    //}}}

    // actions
    void togglePlay() { mTransportStream->togglePlay(); }
    void toggleShowSubtitle() { mOptions->mShowSubtitle = !mOptions->mShowSubtitle; }
    void toggleShowMotionVectors() { mOptions->mShowMotionVectors = !mOptions->mShowMotionVectors; }

    //{{{
    void skip (int64_t skipPts) {

      int64_t offset = mTransportStream->getSkipOffset (skipPts);
      cLog::log (LOGINFO, fmt::format ("skip:{} offset:{} pos:{}", skipPts, offset, mStreamPos));
      mStreamPos += offset;
      }
    //}}}

    //{{{
    void drop (const vector<string>& dropItems) {
    // drop fileNames

      for (auto& item : dropItems) {
        cLog::log (LOGINFO, fmt::format ("cPlayerApp::drop {}", item));
        addFile (item, mOptions);
        }
      }
    //}}}

  private:
    cPlayerOptions* mOptions;

    string mFileName;
    FILE* mFile = nullptr;
    cTransportStream* mTransportStream = nullptr;
    int64_t mStreamPos = 0;
    int64_t mFilePos = 0;
    size_t mFileSize = 0;
    };
  //}}}
  //{{{
  class cPlayerUI : public cApp::iUI {
  public:
    virtual ~cPlayerUI() = default;

    //{{{
    void draw (cApp& app) {

      ImGui::SetKeyboardFocusHere();

      app.getGraphics().clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                                (int32_t)ImGui::GetIO().DisplaySize.y});

      // draw UI
      ImGui::SetNextWindowPos ({0.f,0.f});
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
      ImGui::Begin ("player", nullptr, ImGuiWindowFlags_NoTitleBar |
                                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                       ImGuiWindowFlags_NoBackground);

      cPlayerApp& playerApp = (cPlayerApp&)app;
      if (playerApp.hasTransportStream()) {
        cTransportStream& transportStream = playerApp.getTransportStream();

        // draw multiView piccies
        mMultiView.draw (playerApp, transportStream);

        // draw menu
        ImGui::SetCursorPos ({0.f, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeight() * 1.5f});
        ImGui::BeginChild ("menu", {0.f, ImGui::GetTextLineHeight() * 1.5f},
                           ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoBackground);

        ImGui::SetCursorPos ({0.f,0.f});
        //{{{  draw subtitle button
        ImGui::SameLine();
        if (toggleButton ("sub", playerApp.getOptions()->mShowSubtitle))
          playerApp.toggleShowSubtitle();
        //}}}
        if (playerApp.getOptions()->mHasMotionVectors) {
          //{{{  draw motionVectors button
          ImGui::SameLine();
          if (toggleButton ("motion", playerApp.getOptions()->mShowMotionVectors))
            playerApp.toggleShowMotionVectors();
          }
          //}}}
        if (playerApp.getPlatform().hasFullScreen()) {
          //{{{  draw fullScreen button
          ImGui::SameLine();
          if (toggleButton ("full", playerApp.getPlatform().getFullScreen()))
            playerApp.getPlatform().toggleFullScreen();
          }
          //}}}
        if (playerApp.getPlatform().hasVsync()) {
          //{{{  draw vsync button
          ImGui::SameLine();
          if (toggleButton ("vsync", playerApp.getPlatform().getVsync()))
            playerApp.getPlatform().toggleVsync();
          }
          //}}}
        //{{{  draw frameRate info
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
        //}}}
        ImGui::EndChild();
        }
      ImGui::End();

      keyboard (playerApp);
      }
    //}}}

  private:
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
      bool draw (cPlayerApp& playerApp, cTransportStream& transportStream,
                 bool selectFull, size_t viewIndex, size_t numViews,
                 cTextureShader* videoShader, cTextureShader* subtitleShader) {
      // return true if hit

        (void)transportStream;
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
                           ImGuiChildFlags_None,
                           ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                           ImGuiWindowFlags_NoBackground);
        //{{{  draw select rect
        bool hover = ImGui::IsMouseHoveringRect (mTL, mBR);
        if ((hover || (mSelect != eUnselected)) && (mSelect != eSelectedFull))
          ImGui::GetWindowDrawList()->AddRect (mTL, mBR, hover ? 0xff20ffff : 0xff20ff20, 4.f, 0, 4.f);
        //}}}

        bool enabled = mService.getStream (eVideo).isEnabled();
        if (enabled) {
          //{{{  get audio playPts
          int64_t playPts = mService.getStream (eAudio).getRender().getPts();
          if (mService.getStream (eAudio).isEnabled()) {
            // get playPts from audioStream
            cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (eAudio).getRender());
            if (audioRender.getPlayer())
              playPts = audioRender.getPlayer()->getPts();
            }
          //}}}
          if (!selectFull || (mSelect != eUnselected)) {
            cVideoRender& videoRender = dynamic_cast<cVideoRender&>(mService.getStream (eVideo).getRender());
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
              cTexture& texture = videoFrame->getTexture (playerApp.getGraphics());
              texture.setSource();

              // ensure quad is created
              if (!mVideoQuad)
                mVideoQuad = playerApp.getGraphics().createQuad (videoFrame->getSize());

              // draw quad
              mVideoQuad->draw();

              if (playerApp.getOptions()->mShowSubtitle) {
                //{{{  draw subtitles
                cSubtitleRender& subtitleRender =
                  dynamic_cast<cSubtitleRender&> (mService.getStream (eSubtitle).getRender());

                subtitleShader->use();
                for (size_t line = 0; line < subtitleRender.getNumLines(); line++) {
                  cSubtitleImage& subtitleImage = subtitleRender.getImage (line);
                  if (!mSubtitleTextures[line])
                    mSubtitleTextures[line] = playerApp.getGraphics().createTexture (cTexture::eRgba, subtitleImage.getSize());
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
                    mSubtitleQuads[line] = playerApp.getGraphics().createQuad (mSubtitleTextures[line]->getSize());
                  mSubtitleQuads[line]->draw();
                  }
                }
                //}}}

              if (playerApp.getOptions()->mShowMotionVectors && (mSelect == eSelectedFull)) {
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
            if (mService.getStream (eAudio).isEnabled()) {
              //{{{  audio mute, audioMeter, framesGraphic
              cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (eAudio).getRender());

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

        // largeFont for title and pts
        ImGui::PushFont (playerApp.getLargeFont());
        string title = mService.getName();
        if (!enabled || (mSelect == eSelectedFull))
          title += " " + mService.getNowTitleString();
        //{{{  draw dropShadow title topLeft
        ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
        ImGui::SetCursorPos (pos);
        ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
        ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
        ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
        pos.y += ImGui::GetTextLineHeight() * 1.5f;
        //}}}
        if (mSelect == eSelectedFull) {
          //{{{  draw ptsFromStart bottomRight
          string ptsFromStartString = utils::getPtsString (mService.getPtsFromStart());
          pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
          ImGui::SetCursorPos (pos);
          ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsFromStartString.c_str());
          ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
          ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsFromStartString.c_str());
          }
          //}}}
        ImGui::PopFont();

        ImVec2 viewSubSize = mSize - ImVec2(0.f, ImGui::GetTextLineHeight() *
                                                   ((layoutPos.y + (layoutScale/2.f) >= 0.99f) ? 3.f : 1.5f));

        // invisbleButton over view sub area
        ImGui::SetCursorPos ({0.f,0.f});
        if (ImGui::InvisibleButton (fmt::format ("viewBox##{}", mService.getSid()).c_str(), viewSubSize)) {
          //{{{  hit view, select action
          if (!mService.getStream (eVideo).isEnabled()) {
            mService.enableStream (eVideo);
            mService.enableStream (eAudio);
            mService.enableStream (eSubtitle);
            }
          else if (mSelect == eUnselected)
            mSelect = eSelected;
          else if (mSelect == eSelected)
            mSelect = eSelectedFull;
          else if (mSelect == eSelectedFull)
            mSelect = eSelected;

          result = true;
          }
          //}}}

        ImGui::EndChild();
        return result;
        }
      //}}}

    private:
      static inline const size_t kMaxSubtitleLines = 4;
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
      void draw (cPlayerApp& playerApp, cTransportStream& transportStream) {

        // create shaders, firsttime we see graphics interface
        if (!mVideoShader)
          mVideoShader = playerApp.getGraphics().createTextureShader (cTexture::eYuv420);
        if (!mSubtitleShader)
          mSubtitleShader = playerApp.getGraphics().createTextureShader (cTexture::eRgba);

        // update viewMap from enabled services, take care to reuse cView's
        for (auto& pair : transportStream.getServiceMap()) {
          // find service sid in viewMap
          auto it = mViewMap.find (pair.first);
          if (it == mViewMap.end()) {
            cTransportStream::cService& service = pair.second;
            if (service.getStream (eVideo).isDefined())
              // enabled and not found, add service to viewMap
              mViewMap.emplace (service.getSid(), cView (service));
            }
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
            if (view.second.draw (playerApp, transportStream,
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

    //{{{
    void hitSpace (cPlayerApp& playerApp) {
      playerApp.togglePlay();
      }
    //}}}
    //{{{
    void hitControlLeft (cPlayerApp& playerApp) {
      playerApp.skip (-90000 / 25);
      }
    //}}}
    //{{{
    void hitControlRight (cPlayerApp& playerApp) {
      playerApp.skip (90000 / 25);
      }
    //}}}
    //{{{
    void hitLeft (cPlayerApp& playerApp) {
      playerApp.skip (-90000);
      }
    //}}}
    //{{{
    void hitRight (cPlayerApp& playerApp) {
      playerApp.skip (90000 / 25);
      }
    //}}}
    //{{{
    void hitUp (cPlayerApp& playerApp) {
      playerApp.skip (-900000 / 25);
      }
    //}}}
    //{{{
    void hitDown (cPlayerApp& playerApp) {
      playerApp.skip (900000 / 25);
      }
    //}}}
    //{{{
    void keyboard (cPlayerApp& playerApp) {

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
        { false, false,  false, ImGuiKey_Space,      [this,&playerApp]{ hitSpace (playerApp); }},
        { false, true,   false, ImGuiKey_LeftArrow,  [this,&playerApp]{ hitControlLeft (playerApp); }},
        { false, true,   false, ImGuiKey_RightArrow, [this,&playerApp]{ hitControlRight (playerApp); }},
        { false, false,  false, ImGuiKey_LeftArrow,  [this,&playerApp]{ hitLeft (playerApp); }},
        { false, false,  false, ImGuiKey_RightArrow, [this,&playerApp]{ hitRight (playerApp); }},
        { false, false,  false, ImGuiKey_UpArrow,    [this,&playerApp]{ hitUp (playerApp); }},
        { false, false,  false, ImGuiKey_DownArrow,  [this,&playerApp]{ hitDown (playerApp); }},
        { false, false,  false, ImGuiKey_Enter,      [this,&playerApp]{ mMultiView.hitEnter(); }},
        { false, false,  false, ImGuiKey_F,          [this,&playerApp]{ playerApp.getPlatform().toggleFullScreen(); }},
        { false, false,  false, ImGuiKey_S,          [this,&playerApp]{ playerApp.toggleShowSubtitle(); }},
        { false, false,  false, ImGuiKey_L,          [this,&playerApp]{ playerApp.toggleShowMotionVectors(); }},
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

      ImGui::GetIO().InputQueueCharacters.resize (0);
      }
    //}}}

    // vars
    cMultiView mMultiView;
    };
  //}}}
  }

// main
int main (int numArgs, char* args[]) {

  // options
  cPlayerOptions* options = new cPlayerOptions();
  //{{{  parse commandLine options
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) // found cApp option
      ;
    else if (param == "sub")
      options->mShowSubtitle = true;
    else if (param == "motion")
      options->mHasMotionVectors = true;
    else
      options->mFileName = param;
    }
  //}}}

  // log
  cLog::init (options->mLogLevel);
  cLog::log (LOGNOTICE, fmt::format ("player {} {}",
                                     (dynamic_cast<cApp::cOptions*>(options))->cApp::cOptions::getString(),
                                     options->cPlayerOptions::getString()));

  // launch
  if (!options->mFileName.empty() &&
      options->mFileName.substr (options->mFileName.size() - 3, 3) == ".ts") {
    cPlayerApp playerApp (options, new cPlayerUI());
    playerApp.addFile (options->mFileName, options);
    playerApp.mainUILoop();
    }
  else {
    cSongApp songApp (options, new cSongUI());
    songApp.setSongName (options->mFileName);
    songApp.mainUILoop();
    }

  return EXIT_SUCCESS;
  }

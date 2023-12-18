// tellyApp.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <thread>

#include <sys/stat.h>

//{{{  include stb
// invoke header only library implementation here
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

  #define STB_IMAGE_IMPLEMENTATION
  #include <stb_image.h>

  #define STB_IMAGE_WRITE_IMPLEMENTATION
  #include <stb_image_write.h>

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}

// utils
#include "../common/date.h"
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
#include "../app/myImgui.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// decoders
//{{{  libav
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
#include "../decoders/cSubtitleFrame.h"
#include "../decoders/cSubtitleImage.h"

// song
#include "../song/cSong.h"
#include "../song/cSongLoader.h"

// dvb
#include "../dvb/cDvbMultiplex.h"
#include "../dvb/cDvbSource.h"
#include "../dvb/cTransportStream.h"
#include "../dvb/cVideoRender.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cPlayer.h"

using namespace std;
//}}}
namespace {
  //{{{
  class cTellyOptions : public cApp::cOptions {
  public:
    cTellyOptions() : cOptions() {}
    virtual ~cTellyOptions() = default;

    bool mHasAudio = true;
    bool mHasMotionVectors = false;

    bool mPlaySong = false;

    bool mRecordAll = false;
    bool mShowAllServices = true;
    bool mShowSubtitle = false;
    };
  //}}}

  // song
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
  const vector<string> kDvb  = {"dvb"};

  const vector<string> kRtp1  = {"rtp 1"};
  const vector<string> kRtp2  = {"rtp 2"};
  const vector<string> kRtp3  = {"rtp 3"};
  const vector<string> kRtp4  = {"rtp 4"};
  const vector<string> kRtp5  = {"rtp 5"};
  //}}}
  //{{{
  class cSongApp : public cApp {
  public:
    //{{{
    cSongApp (iUI* ui, const cPoint& windowSize, cTellyOptions* options) :
        cApp(ui, "songApp",windowSize, options) {

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
    cTellyOptions* mOptions;
    cSongLoader* mSongLoader;
    string mSongName;
    };
  //}}}
  //{{{
  class cSongUI : public iUI {
  public:
    virtual void draw (cApp& app) final {
      cSongApp& songApp = (cSongApp&)app;

      ImGui::SetNextWindowPos (ImVec2(0,0));
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
      ImGui::Begin ("song", &mOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar);

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
      //{{{  draw clockButton
      ImGui::SetCursorPos ({ImGui::GetWindowWidth() - 130.f, 0.f});
      clockButton ("clock", app.getNow(), {110.f,110.f});
      //}}}

      // draw song
      if (isDrawMonoSpaced())
        ImGui::PushFont (app.getMonoFont());

      mDrawSong.draw (songApp.getSong(), isDrawMonoSpaced());

      if (isDrawMonoSpaced())
        ImGui::PopFont();

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

    bool mOpen = true;
    bool mShowMonoSpaced = false;
    };
  //}}}

  // telly
  //{{{
  const vector <cDvbMultiplex> kDvbMultiplexes = {
      { "file", false, 0, {}, {}, {} },  // dummy file multiplex spec

      { "hd", 626000000, false,
        { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
        { "bbc1",          "bbc2",       "bbc3",         "bbc4",        "itv",     "c4",           "c5" },
        { 1,               2,            3,              4,             6,         7,              5 }
      },
      { "itv", 650000000, false,
        { "ITV1",   "ITV2", "ITV3", "ITV4", "Channel 4", "Channel 4+1", "More 4", "Film4" , "E4", "Channel 5" },
        { "itv1sd", "itv2", "itv3", "itv4", "chn4sd"   , "c4+1",        "more4",  "film4",  "e4", "chn5sd" },
        { 1,        2,      3,      4,      5,           6,             7,        8,        9,    0 }
      },

      { "bbc", 674000000, false,
        { "BBC ONE S West", "BBC TWO", "BBC FOUR" },
        { "bbc1sd",         "bbc2sd",  "bbc4sd" },
        { 1,                2,         4 }
      }
    };
  //}}}
  //{{{  const string kRootDir
  #ifdef _WIN32
    const string kRootDir = "/tv/";
  #else
    const string kRootDir = "/home/pi/tv/";
  #endif
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

    uint16_t getId() const { return mService.getSid(); }
    bool getHover() const { return mHover; }

    // verbose selectState gets
    bool getUnselected() const { return mSelect == eUnselected; }
    bool getSelected() const { return mSelect != eUnselected; }
    bool getSelectedFull() const { return mSelect == eSelectedFull; }

    bool pick (cVec2 pos) { return mRect.isInside (pos); }
    //{{{
    bool hover() {
      mHover = ImGui::IsMouseHoveringRect (ImVec2 ((float)mRect.left, (float)mRect.top),
                                           ImVec2 ((float)mRect.right, (float)mRect.bottom));
      return mHover;
      }
    //}}}
    //{{{
    bool select (uint16_t id) {

      if (id != getId())
        mSelect = eUnselected;
      else if (mSelect == eUnselected)
        mSelect = eSelected;
      else if (mSelect == eSelectedFull)
        mSelect = eSelected;
      else if (mSelect == eSelected) {
        mSelect = eSelectedFull;
        return true;
        }

      return false;
      }
    //}}}

    //{{{
    void draw (cGraphics& graphics, bool selectFull, size_t numViews, bool drawSubtitle, size_t viewIndex) {

      cVideoRender& videoRender = dynamic_cast<cVideoRender&> (
        mService.getRenderStream (eRenderVideo).getRender());

      int64_t playPts = mService.getRenderStream (eRenderAudio).getPts();
      if (mService.getRenderStream (eRenderAudio).isEnabled()) {
        //{{{  get playPts from audioStream
        cAudioRender& audioRender = dynamic_cast<cAudioRender&>(
          mService.getRenderStream (eRenderAudio).getRender());

        if (audioRender.getPlayer())
          playPts = audioRender.getPlayer()->getPts();
        }
        //}}}

      if (!selectFull || getSelected()) {
        // view selected or no views selected
        float viewportWidth = ImGui::GetWindowWidth();
        float viewportHeight = ImGui::GetWindowHeight();
        cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);

        float scale = getScale (numViews);

        // show nearest videoFrame to playPts
        cVideoFrame* videoFrame = videoRender.getVideoFrameAtOrAfterPts (playPts);
        if (videoFrame) {
          //{{{  draw video
          mVideoShader->use();

          // texture
          cTexture& texture = videoFrame->getTexture (graphics);
          texture.setSource();

          // model
          mModel = cMat4x4();
          cVec2 fraction = gridFractionalPosition (viewIndex, numViews);
          mModel.setTranslate ({ (fraction.x - (0.5f * scale)) * viewportWidth,
                                 (fraction.y - (0.5f * scale)) * viewportHeight });
          mModel.size ({ scale * viewportWidth / videoFrame->getWidth(),
                         scale * viewportHeight / videoFrame->getHeight() });
          mVideoShader->setModelProjection (mModel, projection);

          // ensure quad is created and drawIt
          if (!mVideoQuad)
            mVideoQuad = graphics.createQuad (videoFrame->getSize());

          mVideoQuad->draw();

          mRect = cRect (mModel.transform (cVec2(0, videoFrame->getHeight()), viewportHeight),
                         mModel.transform (cVec2(videoFrame->getWidth(), 0), viewportHeight));

          mVideoQuad = graphics.createQuad (videoFrame->getSize());

          for (auto& motionVector : videoFrame->getMotionVectors())
            ImGui::GetWindowDrawList()->AddLine (
              { (float)mRect.left + motionVector.mSrcx, (float)mRect.top + motionVector.mSrcy },
              { (float)mRect.left + motionVector.mDstx, (float)mRect.top + motionVector.mDsty},
              motionVector.mSource > 0 ? 0xc0c0c0c0 : 0xc000c0c0, 1.f);
          //}}}

          if ((getHover() || getSelected()) && !getSelectedFull())
            //{{{  draw select rectangle
            ImGui::GetWindowDrawList()->AddRect ({ (float)mRect.left, (float)mRect.top },
                                                 { (float)mRect.right, (float)mRect.bottom },
                                                 getHover() ? 0xff20ffff : 0xff20ff20, 4.f, 0, 4.f);
            //}}}

          if (drawSubtitle) {
            cSubtitleRender& subtitleRender = dynamic_cast<cSubtitleRender&> (
              mService.getRenderStream (eRenderSubtitle).getRender());
            if (mService.getRenderStream (eRenderAudio).isEnabled()) {
              //{{{  draw subtitles
              mSubtitleShader->use();

              for (size_t line = 0; line < subtitleRender.getNumLines(); line++) {
                cSubtitleImage& subtitleImage = subtitleRender.getImage (line);

                if (!mSubtitleTextures[line])
                  mSubtitleTextures[line] = graphics.createTexture (cTexture::eRgba, subtitleImage.getSize());
                mSubtitleTextures[line]->setSource();

                // update subtitle texture if image dirty
                if (subtitleImage.isDirty())
                  mSubtitleTextures[line]->setPixels (subtitleImage.getPixels(), nullptr);
                subtitleImage.setDirty (false);

                float xpos = (float)subtitleImage.getXpos() / videoFrame->getWidth();
                float ypos = (float)(videoFrame->getHeight() - subtitleImage.getYpos()) / videoFrame->getHeight();
                mModel.setTranslate ({ (fraction.x + ((xpos - 0.5f) * scale)) * viewportWidth,
                                       (fraction.y + ((ypos - 0.5f) * scale)) * viewportHeight });
                mSubtitleShader->setModelProjection (mModel, projection);

                // ensure quad is created (assumes same size) and drawIt
                if (!mSubtitleQuads[line])
                  mSubtitleQuads[line] = graphics.createQuad (mSubtitleTextures[line]->getSize());
                mSubtitleQuads[line]->draw();
                }
              }
              //}}}
            }
          }

        if (mService.getRenderStream (eRenderAudio).isEnabled()) {
          //{{{  mute, draw audioMeter, draw framesGraphic
          cAudioRender& audioRender = dynamic_cast<cAudioRender&>(
            mService.getRenderStream (eRenderAudio).getRender());

          if (audioRender.getPlayer())
            audioRender.getPlayer()->setMute (getUnselected());

          // draw audio meter
          mAudioMeterView.draw (audioRender, playPts,
                                ImVec2((float)mRect.right - (0.5f * ImGui::GetTextLineHeight()),
                                       (float)mRect.bottom - (0.5f * ImGui::GetTextLineHeight())));
          if (getSelectedFull())
            mFramesView.draw (audioRender, videoRender, playPts,
                              ImVec2((float)mRect.getCentre().x,
                                     (float)mRect.bottom - (0.5f * ImGui::GetTextLineHeight())));
          }
          //}}}

        //{{{  draw service title
        string title = mService.getChannelName();
        if (getSelectedFull())
          title += " " + mService.getNowTitleString();

        ImGui::SetCursorPos ({(float)mRect.left + (ImGui::GetTextLineHeight() * 0.25f),
                              (float)mRect.bottom - ImGui::GetTextLineHeight()});
        ImGui::TextColored ({0.f, 0.f,0.f,1.f}, title.c_str());

        ImGui::SetCursorPos ({(float)mRect.left - 2.f + (ImGui::GetTextLineHeight() * 0.25f),
                              (float)mRect.bottom - 2.f - ImGui::GetTextLineHeight()});
        ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
        //}}}
        }
      }
    //}}}

    inline static cTextureShader* mVideoShader = nullptr;
    inline static cTextureShader* mSubtitleShader = nullptr;

  private:
    enum eSelect { eUnselected, eSelected, eSelectedFull };
    //{{{
    class cFramesView {
    public:
      cFramesView() {}
      ~cFramesView() = default;

      //{{{
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
      //}}}

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
      cAudioMeterView() {}
      ~cAudioMeterView() = default;

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
              { x, pos.y - (audioFrame->mPowerValues[channelOrder[i]] * height) },
              { x + mPixelsPerAudioChannel - 1.f, pos.y },
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
    float getScale (size_t numViews) const {
      return (numViews <= 1) ? 1.f :
        ((numViews <= 4) ? 0.5f :
          ((numViews <= 9) ? 0.33f :
            ((numViews <= 16) ? 0.25f : 0.20f)));
      }
    //}}}
    //{{{
    cVec2 gridFractionalPosition (size_t index, size_t numViews) {

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
            case 0: return { 1.f / 4.f, 3.f / 4.f };
            case 1: return { 3.f / 4.f, 3.f / 4.f };

            case 2: return { 1.f / 4.f, 1.f / 4.f };
            case 3: return { 3.f / 4.f, 1.f / 4.f };
            }
          return { 0.5f, 0.5f };
        //}}}

        case 5:
        //{{{
        case 6: // 3x2
          switch (index) {
            case 0: return { 1.f / 6.f, 4.f / 6.f };
            case 1: return { 3.f / 6.f, 4.f / 6.f };
            case 2: return { 5.f / 6.f, 4.f / 6.f };

            case 3: return { 1.f / 6.f, 2.f / 6.f };
            case 4: return { 3.f / 6.f, 2.f / 6.f };
            case 5: return { 5.f / 6.f, 2.f / 6.f };
            }
          return { 0.5f, 0.5f };
        //}}}

        case 7:
        case 8:
        //{{{
        case 9: // 3x3
          switch (index) {
            case 0: return { 1.f / 6.f, 5.f / 6.f };
            case 1: return { 3.f / 6.f, 5.f / 6.f };
            case 2: return { 5.f / 6.f, 5.f / 6.f };

            case 3: return { 1.f / 6.f, 0.5f };
            case 4: return { 3.f / 6.f, 0.5f };
            case 5: return { 5.f / 6.f, 0.5f };

            case 6: return { 1.f / 6.f, 1.f / 6.f };
            case 7: return { 3.f / 6.f, 1.f / 6.f };
            case 8: return { 5.f / 6.f, 1.f / 6.f };
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
            case  0: return { 1.f / 8.f, 7.f / 8.f };
            case  1: return { 3.f / 8.f, 7.f / 8.f };
            case  2: return { 5.f / 8.f, 7.f / 8.f };
            case  3: return { 7.f / 8.f, 7.f / 8.f };

            case  4: return { 1.f / 8.f, 5.f / 8.f };
            case  5: return { 3.f / 8.f, 5.f / 8.f };
            case  6: return { 5.f / 8.f, 5.f / 8.f };
            case  7: return { 7.f / 8.f, 5.f / 8.f };

            case  8: return { 1.f / 8.f, 3.f / 8.f };
            case  9: return { 3.f / 8.f, 3.f / 8.f };
            case 10: return { 5.f / 8.f, 3.f / 8.f };
            case 11: return { 7.f / 8.f, 3.f / 8.f };

            case 12: return { 1.f / 8.f, 1.f / 8.f };
            case 13: return { 3.f / 8.f, 1.f / 8.f };
            case 14: return { 5.f / 8.f, 1.f / 8.f };
            case 15: return { 7.f / 8.f, 1.f / 8.f };
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

    bool mHover = false;
    eSelect mSelect = eUnselected;

    // video
    cMat4x4 mModel;
    cRect mRect = { 0,0,0,0 };
    cQuad* mVideoQuad = nullptr;

    // subtitle
    static const size_t kMaxSubtitleLines = 4;
    array <cQuad*, kMaxSubtitleLines> mSubtitleQuads = { nullptr };
    array <cTexture*, kMaxSubtitleLines> mSubtitleTextures = { nullptr };

    cFramesView mFramesView;
    cAudioMeterView mAudioMeterView;
    };
  //}}}
  //{{{
  class cMultiView {
  public:
    size_t getNumViews() const { return mViewMap.size(); }

    //{{{
    bool hover() {
    // run for every item to ensure mHover flag get set

      bool result = false;

      for (auto& view : mViewMap)
        if (view.second.hover())
          result = true;;

      return result;
      }
    //}}}
    //{{{
    uint16_t pick (cVec2 mousePosition) {
    // pick first view that matches

      for (auto& view : mViewMap)
        if (view.second.pick (mousePosition))
          return view.second.getId();

      return 0;
      }
    //}}}
    //{{{
    void selectById (uint16_t id) {
    // select if id nonZero

      if (id) {
        bool selectFull = false;
        for (auto& view : mViewMap)
          selectFull |= view.second.select (id);

        mSelectFull = selectFull;
        }
      }
    //}}}

    //{{{
    void moveLeft() {
      cLog::log (LOGINFO, fmt::format ("moveLeft"));
      }
    //}}}
    //{{{
    void moveRight() {
      cLog::log (LOGINFO, fmt::format ("moveRight"));
      }
    //}}}
    //{{{
    void moveUp() {
      cLog::log (LOGINFO, fmt::format ("moveUp"));
      }
    //}}}
    //{{{
    void moveDown() {
      cLog::log (LOGINFO, fmt::format ("moveDown"));
      }
    //}}}
    //{{{
    void enter() {
      cLog::log (LOGINFO, fmt::format ("enter"));
      }
    //}}}
    //{{{
    void space() {
      cLog::log (LOGINFO, fmt::format ("space"));
      }
    //}}}

    //{{{
    void draw (cTransportStream& transportStream, cGraphics& graphics, bool drawSubtitle) {

      if (!cView::mVideoShader)
       cView::mVideoShader = graphics.createTextureShader (cTexture::eYuv420);

      if (!cView::mSubtitleShader)
        cView::mSubtitleShader = graphics.createTextureShader (cTexture::eRgba);

      // update viewMap from enabled services, take care to reuse cView's
      for (auto& pair : transportStream.getServiceMap()) {
        cTransportStream::cService& service = pair.second;

        // find service sid in viewMap
        auto it = mViewMap.find (pair.first);
        if (it == mViewMap.end()) {
          if (service.getRenderStream (eRenderVideo).isEnabled())
            // enabled and not found, add service to viewMap
            mViewMap.emplace (service.getSid(), cView (service));
          }
        else if (!service.getRenderStream (eRenderVideo).isEnabled())
          // found, but not enabled, remove service from viewMap
          mViewMap.erase (it);
        }

      // draw views
      size_t viewIndex = 0;
      for (auto& view : mViewMap)
        view.second.draw (graphics, mSelectFull, mSelectFull ? 1 : getNumViews(), drawSubtitle, viewIndex++);
      }
    //}}}

  private:
    map <uint16_t, cView> mViewMap;

    bool mSelectFull = false;
    };
  //}}}
  //{{{
  class cTellyApp : public cApp {
  public:
    cTellyApp (iUI* ui, const cPoint& windowSize, cTellyOptions* options) :
      cApp (ui, "telly", windowSize, options), mOptions(options) {}
    virtual ~cTellyApp() = default;

    cMultiView& getMultiView() { return mMultiView; }

    bool hasTransportStream() { return mTransportStream; }
    cTransportStream& getTransportStream() { return *mTransportStream; }

    bool showSubtitle() const { return mOptions->mShowSubtitle; }
    void toggleShowSubtitle() { mOptions->mShowSubtitle = !mOptions->mShowSubtitle; }

    // fileSource
    bool isFileSource() const { return !mFileName.empty(); }
    std::string getFileName() const { return mFileName; }
    uint64_t getFilePos() const { return mFilePos; }
    size_t getFileSize() const { return mFileSize; }
    //{{{
    void fileSource (const string& filename, bool showAllServices) {
    // create fileSource, any channel

      mTransportStream = new cTransportStream (kDvbMultiplexes[0], "", false, showAllServices, true,
                                               mOptions->mHasAudio, mOptions->mHasMotionVectors);
      if (mTransportStream) {
        // launch fileSource thread
        mFileName = cFileUtils::resolve (filename);
        FILE* file = fopen (mFileName.c_str(), "rb");
        if (file) {
          // create fileRead thread
          thread ([=]() {
            cLog::setThreadName ("file");

            size_t blockSize = 188 * 256;
            uint8_t* buffer = new uint8_t[blockSize];

            mFilePos = 0;
            while (true) {
              size_t bytesRead = fread (buffer, 1, blockSize, file);
              if (bytesRead > 0)
                mFilePos += mTransportStream->demux (buffer, bytesRead, mFilePos, false);
              else
                break;
              //{{{  get mFileSize
              #ifdef _WIN32
                // windows platform nonsense
                struct _stati64 st;
                if (_stat64 (mFileName.c_str(), &st) != -1)
              #else
                struct stat st;
                if (stat (mFileName.c_str(), &st) != -1)
              #endif
                  mFileSize = st.st_size;
              //}}}
              }

            fclose (file);
            delete [] buffer;
            cLog::log (LOGERROR, "exit");
            }).detach();
          return;
          }

        cLog::log (LOGERROR, fmt::format ("cTellyApp::fileSource failed to open{}", mFileName));
        }
      else
        cLog::log (LOGERROR, "cTellyApp::cTransportStream create failed");
      }
    //}}}

    // liveDvbSource
    bool isDvbSource() const { return mDvbSource; }
    cDvbSource& getDvbSource() { return *mDvbSource; }
    //{{{
    void liveDvbSource (const cDvbMultiplex& multiplex, const string& recordRoot, bool showAllServices) {
    // create liveDvbSource from dvbMultiplex

      cLog::log (LOGINFO, fmt::format ("using multiplex {} {:4.1f} record {} {}",
                                       multiplex.mName, multiplex.mFrequency/1000.f,
                                       recordRoot, showAllServices ? "all " : ""));

      mMultiplex = multiplex;
      mRecordRoot = recordRoot;
      if (multiplex.mFrequency)
        mDvbSource = new cDvbSource (multiplex.mFrequency, 0);

      mTransportStream = new cTransportStream (multiplex, recordRoot, true, showAllServices, false,
                                               mOptions->mHasAudio, mOptions->mHasMotionVectors);
      if (mTransportStream) {
        mLiveThread = thread ([=]() {
          cLog::setThreadName ("dvb");

          FILE* mFile = mMultiplex.mRecordAll ?
            fopen ((mRecordRoot + mMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

          #ifdef _WIN32
            //{{{  windows
            // raise thread priority
            SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

            mDvbSource->run();

            int64_t streamPos = 0;
            int blockSize = 0;

            while (true) {
              auto ptr = mDvbSource->getBlockBDA (blockSize);
              if (blockSize) {
                //  read and demux block
                streamPos += mTransportStream->demux (ptr, blockSize, streamPos, false);
                if (mMultiplex.mRecordAll && mFile)
                  fwrite (ptr, 1, blockSize, mFile);
                mDvbSource->releaseBlock (blockSize);
                }
              else
                this_thread::sleep_for (1ms);
              }
            //}}}
          #else
            //{{{  linux
            // raise thread priority
            sched_param sch_params;
            sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
            pthread_setschedparam (mLiveThread.native_handle(), SCHED_RR, &sch_params);

            constexpr int kDvrReadBufferSize = 188 * 64;
            uint8_t* buffer = new uint8_t[kDvrReadBufferSize];

            uint64_t streamPos = 0;
            while (true) {
              int bytesRead = mDvbSource->getBlock (buffer, kDvrReadBufferSize);
              if (bytesRead) {
                streamPos += mTransportStream->demux (buffer, bytesRead, 0, false);
                if (mMultiplex.mRecordAll && mFile)
                  fwrite (buffer, 1, bytesRead, mFile);
                }
              else
                cLog::log (LOGINFO, fmt::format ("cTellyApp::liveDvbSource - no bytes"));
              }

            delete [] buffer;
            //}}}
          #endif

          if (mFile)
            fclose (mFile);

          cLog::log (LOGINFO, "exit");
          });
        mLiveThread.detach();
        }

      else
        cLog::log (LOGINFO, "cTellyApp::setLiveDvbSource - failed to create liveDvbSource");
      }
    //}}}

    //{{{
    void drop (const vector<string>& dropItems) {
    // drop fileSource

      for (auto& item : dropItems) {
        cLog::log (LOGINFO, fmt::format ("cTellyApp::drop {}", item));
        fileSource (item, true);
        }
      }
    //}}}

  private:
    cTellyOptions* mOptions;
    cTransportStream* mTransportStream = nullptr;

    cMultiView mMultiView;

    // fileSource
    FILE* mFile = nullptr;
    std::string mFileName;
    uint64_t mFilePos = 0;
    size_t mFileSize = 0;

    // liveDvbSource
    thread mLiveThread;
    cDvbSource* mDvbSource = nullptr;
    cDvbMultiplex mMultiplex;
    string mRecordRoot;
    };
  //}}}
  //{{{
  class cTellyUI : public iUI {
  public:
    //{{{
    virtual void draw (cApp& app) final {

      cTellyApp& tellyApp = (cTellyApp&)app;
      app.getGraphics().clear ({ (int32_t)ImGui::GetIO().DisplaySize.x,
                                 (int32_t)ImGui::GetIO().DisplaySize.y });

      ImGui::SetKeyboardFocusHere();
      ImGui::SetNextWindowPos ({ 0.f,0.f });
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
      ImGui::Begin ("telly", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar);

      // draw multiView
      if (tellyApp.hasTransportStream())
        tellyApp.getMultiView().draw (tellyApp.getTransportStream(), app.getGraphics(), tellyApp.showSubtitle());

      drawTabs (tellyApp);
      //{{{  draw subtitle button
      ImGui::SameLine();
      if (toggleButton ("sub", tellyApp.showSubtitle()))
        tellyApp.toggleShowSubtitle();
      //}}}
      //{{{  draw fullScreen button
      if (app.getPlatform().hasFullScreen()) {
        ImGui::SameLine();
        if (toggleButton ("full", app.getPlatform().getFullScreen()))
          app.getPlatform().toggleFullScreen();
        }
      //}}}
      //{{{  draw vsync button, frameRate info
      ImGui::SameLine();
      if (app.getPlatform().hasVsync())
        if (toggleButton ("vsync", app.getPlatform().getVsync()))
          app.getPlatform().toggleVsync();

      ImGui::SameLine();
      ImGui::TextUnformatted(fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      //}}}

      if (tellyApp.hasTransportStream()) {
        cTransportStream& transportStream = tellyApp.getTransportStream();
        if (transportStream.hasTdtTime()) {
          //{{{  draw tdtTime info
          ImGui::SameLine();
          ImGui::TextUnformatted (transportStream.getTdtTimeString().c_str());
          }
          //}}}
        if (tellyApp.isFileSource()) {
          //{{{  draw filePos info
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{:4.3f}%", tellyApp.getFilePos() * 100.f /
                                                           tellyApp.getFileSize()).c_str());
          }
          //}}}
        else if (tellyApp.isDvbSource()) {
          //{{{  draw dvbSource signal,errors info
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{} {}", tellyApp.getDvbSource().getTuneString(),
                                                        tellyApp.getDvbSource().getStatusString()).c_str());
          }
          //}}}
        if (tellyApp.getTransportStream().getNumErrors()) {
          //{{{  draw transportStream errors info
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("error:{}", tellyApp.getTransportStream().getNumErrors()).c_str());
          }
          //}}}

        // draw tab with monoSpaced font
        ImGui::PushFont (app.getMonoFont());
        switch (mTab) {
          case eChannels:   drawChannels (transportStream); break;
          case eServices:   drawServices (transportStream, app.getGraphics()); break;
          case ePidMap:     drawPidMap (transportStream); break;
          case eRecordings: drawRecordings (transportStream); break;
          default:;
          }
        ImGui::PopFont();

        // invisible bgnd button for mouse
        ImGui::SetCursorPos ({ 0.f,0.f });
        if (ImGui::InvisibleButton ("bgnd", { ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y }))
          tellyApp.getMultiView().selectById (
            tellyApp.getMultiView().pick ({ ImGui::GetMousePos().x, ImGui::GetMousePos().y }));
        tellyApp.getMultiView().hover();

        if (tellyApp.hasTransportStream() && transportStream.hasTdtTime()) {
          //{{{  draw clock
          ImGui::SetCursorPos ({ ImGui::GetWindowWidth() - 90.f, 0.f} );
          clockButton ("clock", transportStream.getTdtTime(), { 80.f, 80.f });
          }
          //}}}

        //if (ImGui::IsWindowHovered())
        //  ImGui::SetMouseCursor (ImGuiMouseCursor_TextInput);
        keyboard (tellyApp);

        ImGui::End();
        }
      }
    //}}}

  private:
    enum eTab { eTelly, eServices, ePidMap, eChannels, eRecordings };
    inline static const vector<string> kTabNamesRecord = { "telly", "services", "pids", "channels", "recorded" };
    inline static const vector<string> kTabNames =       { "telly", "services", "pids", "channels" };
    inline static const vector<string> kTabNamesSingle = { "telly", "services", "pids" };

    //{{{
    void hitShow (eTab tab) {
      mTab = (tab == mTab) ? eTelly : tab;
      }
    //}}}

    //{{{
    void drawTabs (cTellyApp& tellyApp) {

      ImGui::SetCursorPos ({ 0.f,0.f });

      if (tellyApp.hasTransportStream()) {
        if (tellyApp.getTransportStream().getRecordPrograms().size())
          mTab = (eTab)interlockedButtons (kTabNamesRecord, (uint8_t)mTab, {0.f,0.f}, true);
        else if (tellyApp.getTransportStream().getServiceMap().size() > 1)
          mTab = (eTab)interlockedButtons (kTabNames, (uint8_t)mTab, {0.f,0.f}, true);
        else
          mTab = (eTab)interlockedButtons (kTabNamesSingle, (uint8_t)mTab, {0.f,0.f}, true);
        }
      }
    //}}}
    //{{{
    void drawChannels (cTransportStream& transportStream) {

      // draw services/channels
      for (auto& pair : transportStream.getServiceMap()) {
        cTransportStream::cService& service = pair.second;
        if (ImGui::Button (fmt::format ("{:{}s}", service.getChannelName(), mMaxNameChars).c_str())) {
          service.toggleAll();
          }

        if (service.getRenderStream (eRenderAudio).isDefined()) {
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{}{}",
            service.getRenderStream (eRenderAudio).isEnabled() ? "*":"", service.getNowTitleString()).c_str());
          }

        while (service.getChannelName().size() > mMaxNameChars)
          mMaxNameChars = service.getChannelName().size();
        }
      }
    //}}}
    //{{{
    void drawServices (cTransportStream& transportStream, cGraphics& graphics) {

      // iterate services
      for (auto& pair : transportStream.getServiceMap()) {
        cTransportStream::cService& service = pair.second;
        //{{{  update button maxChars for uniform width
        while (service.getChannelName().size() > mMaxNameChars)
          mMaxNameChars = service.getChannelName().size();
        while (service.getSid() > pow (10, mMaxSidChars))
          mMaxSidChars++;
        while (service.getProgramPid() > pow (10, mMaxPgmChars))
          mMaxPgmChars++;

        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
          uint16_t pid = service.getRenderStream (eRenderType(renderType)).getPid();
          while (pid > pow (10, mPidMaxChars[renderType]))
            mPidMaxChars[renderType]++;
          }
        //}}}

        // draw channel name pgm,sid - sid ensures unique button name
        if (ImGui::Button (fmt::format ("{:{}s} {:{}d}:{:{}d}",
                           service.getChannelName(), mMaxNameChars,
                           service.getProgramPid(), mMaxPgmChars, service.getSid(), mMaxSidChars).c_str())) {
          service.toggleAll();
          }

       // iterate definedStreams
        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
          cTransportStream::cStream& stream = service.getRenderStream (eRenderType(renderType));
          if (stream.isDefined()) {
            ImGui::SameLine();
            // draw definedStream button - hidden sid for unique buttonName
            if (toggleButton (fmt::format ("{}{:{}d}:{}##{}",
                                           stream.getLabel(), stream.getPid(),
                                           mPidMaxChars[renderType], stream.getTypeName(),
                                           service.getSid()).c_str(),
                              stream.isEnabled()))
             transportStream.toggleStream (service, eRenderType(renderType));
            }
          }

        if (service.getChannelRecord()) {
          //{{{  draw record pathName
          ImGui::SameLine();
          ImGui::Button (fmt::format ("rec:{}##{}", service.getChannelRecordName(), service.getSid()).c_str());
          }
          //}}}

        // iterate enabledStreams
        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
          cTransportStream::cStream& stream = service.getRenderStream (eRenderType(renderType));
          if (stream.isEnabled()) {
            switch (eRenderType(renderType)) {
              case eRenderVideo:
                drawVideoInfo (service.getSid(), stream.getRender()); break;

              case eRenderAudio:
              case eRenderDescription:
                drawAudioInfo (service.getSid(), stream.getRender());  break;

              case eRenderSubtitle:
                drawSubtitle (service.getSid(), stream.getRender(), graphics);  break;

              default:;
              }
            }
          }
        }
      }
    //}}}
    //{{{
    void drawPidMap (cTransportStream& transportStream) {
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
                                pidInfo.getTypeName()).c_str());

        // draw stream bar
        ImGui::SameLine();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        mMaxPidPackets = max (mMaxPidPackets, pidInfo.mPackets);
        float frac = pidInfo.mPackets / float(mMaxPidPackets);
        ImVec2 posTo = {pos.x + (frac * (ImGui::GetWindowWidth() - pos.x - ImGui::GetTextLineHeight())),
                        pos.y + ImGui::GetTextLineHeight()};
        ImGui::GetWindowDrawList()->AddRectFilled (pos, posTo, 0xff00ffff);

        // draw stream label
        string streamText = pidInfo.getInfoString();
        if ((pidInfo.getStreamType() == 0) && (pidInfo.getSid() != 0xFFFF))
          streamText = fmt::format ("{} ", pidInfo.getSid()) + streamText;
        ImGui::TextUnformatted (streamText.c_str());

        // adjust packet number width
        if (pidInfo.mPackets > pow (10, mPacketChars))
          mPacketChars++;

        prevSid = pidInfo.getSid();
        }
      }
    //}}}
    //{{{
    void drawRecordings (cTransportStream& transportStream) {

      for (auto& program : transportStream.getRecordPrograms())
        ImGui::TextUnformatted (program.c_str());
      }
    //}}}

    //{{{
    void drawSubtitle (uint16_t sid, cRender& render, cGraphics& graphics) {

      cSubtitleRender& subtitleRender = dynamic_cast<cSubtitleRender&>(render);

      const float potSize = ImGui::GetTextLineHeight() / 2.f;
      size_t line;
      for (line = 0; line < subtitleRender.getNumLines(); line++) {
        cSubtitleImage& subtitleImage = subtitleRender.getImage (line);

        // draw clut colorPots
        ImVec2 pos = ImGui::GetCursorScreenPos();
        for (size_t pot = 0; pot < subtitleImage.mColorLut.max_size(); pot++) {
          //{{{  draw pot
          ImVec2 potPos {pos.x + (pot % 8) * potSize, pos.y + (pot / 8) * potSize};
          uint32_t color = subtitleRender.getImage (line).mColorLut[pot];
          ImGui::GetWindowDrawList()->AddRectFilled (potPos, { potPos.x + potSize - 1.f,
                                                               potPos.y + potSize - 1.f }, color);
          }
          //}}}

        if (ImGui::InvisibleButton (fmt::format ("##sub{}{}", sid, line).c_str(),
                                    { 4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight() }))
          subtitleRender.toggleLog();

        // draw pos
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("{:3d},{:3d}",
                                              subtitleImage.getXpos(), subtitleImage.getYpos()).c_str());

        if (!mSubtitleTextures[line])
           mSubtitleTextures[line] = graphics.createTexture (cTexture::eRgba, subtitleImage.getSize());
        mSubtitleTextures[line]->setSource();

        // update gui texture from subtitleImage
        if (subtitleImage.isDirty())
          mSubtitleTextures[line]->setPixels (subtitleImage.getPixels(), nullptr);
        subtitleImage.setDirty (false);

        // draw gui texture, scaled to fit line
        ImGui::SameLine();
        ImGui::Image ((void*)(intptr_t)mSubtitleTextures[line]->getTextureId(),
                      { ImGui::GetTextLineHeight() *
                          mSubtitleTextures[line]->getWidth() / mSubtitleTextures[line]->getHeight(),
                        ImGui::GetTextLineHeight() });
        }

      // pad with invisible buttons to highwaterMark
      for (; line < subtitleRender.getHighWatermark(); line++)
        if (ImGui::InvisibleButton (fmt::format ("##sub{}{}", sid, line).c_str(),
                                    { ImGui::GetWindowWidth() - ImGui::GetTextLineHeight(),
                                      ImGui::GetTextLineHeight() }))
          subtitleRender.toggleLog();

      drawMiniLog (subtitleRender.getLog());
      }
    //}}}
    //{{{
    void drawAudioInfo (uint16_t sid, cRender& render) {

      cAudioRender& audioRender = dynamic_cast<cAudioRender&>(render);
      ImGui::TextUnformatted (audioRender.getInfoString().c_str());

      if (ImGui::InvisibleButton (fmt::format ("##audLog{}", sid).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        audioRender.toggleLog();

      drawMiniLog (audioRender.getLog());
      }
    //}}}
    //{{{
    void drawVideoInfo (uint16_t sid, cRender& render) {

      cVideoRender& videoRender = dynamic_cast<cVideoRender&>(render);
      ImGui::TextUnformatted (videoRender.getInfoString().c_str());

      if (ImGui::InvisibleButton (fmt::format ("##vidLog{}", sid).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        videoRender.toggleLog();

      drawMiniLog (videoRender.getLog());
      }
    //}}}

    //{{{
    void keyboard (cTellyApp& tellyApp) {

      //{{{
      struct sActionKey {
        bool mAlt;
        bool mCtrl;
        bool mShift;
        ImGuiKey mGuiKey;
        function <void()> mActionFunc;
        };
      //}}}
      const vector<sActionKey> kActionKeys = {
        //alt    ctrl   shift  guiKey               function
        { false, false, false, ImGuiKey_LeftArrow,  [this,&tellyApp] { tellyApp.getMultiView().moveLeft(); }},
        { false, false, false, ImGuiKey_RightArrow, [this,&tellyApp] { tellyApp.getMultiView().moveRight(); }},
        { false, false, false, ImGuiKey_UpArrow,    [this,&tellyApp] { tellyApp.getMultiView().moveUp(); }},
        { false, false, false, ImGuiKey_DownArrow,  [this,&tellyApp] { tellyApp.getMultiView().moveDown(); }},
        { false, false, false, ImGuiKey_Enter,      [this,&tellyApp] { tellyApp.getMultiView().enter(); }},
        { false, false, false, ImGuiKey_Space,      [this,&tellyApp] { tellyApp.getMultiView().space(); }},
        { false, false, false, ImGuiKey_F,          [this,&tellyApp] { tellyApp.getPlatform().toggleFullScreen(); }},
        { false, false, false, ImGuiKey_S,          [this,&tellyApp] { tellyApp.toggleShowSubtitle(); }},
        { false, false, false, ImGuiKey_M,          [this,&tellyApp] { hitShow (eTelly); }},
        { false, false, false, ImGuiKey_C,          [this,&tellyApp] { hitShow (eChannels); }},
        { false, false, false, ImGuiKey_V,          [this,&tellyApp] { hitShow (eServices); }},
        { false, false, false, ImGuiKey_P,          [this,&tellyApp] { hitShow (ePidMap); }},
        { false, false, false, ImGuiKey_R,          [this,&tellyApp] { hitShow (eRecordings); }},
        };

      ImGui::GetIO().WantTextInput = true;
      ImGui::GetIO().WantCaptureKeyboard = true;

      // look for action keys
      bool altKeyPressed = ImGui::GetIO().KeyAlt;
      bool ctrlKeyPressed = ImGui::GetIO().KeyCtrl;
      bool shiftKeyPressed = ImGui::GetIO().KeyShift;
      for (auto& actionKey : kActionKeys)
        if (ImGui::IsKeyPressed (actionKey.mGuiKey) &&
            (actionKey.mAlt == altKeyPressed) &&
            (actionKey.mCtrl == ctrlKeyPressed) &&
            (actionKey.mShift == shiftKeyPressed)) {
          actionKey.mActionFunc();
          break;
          }

      // other queued keys
      for (int i = 0; i < ImGui::GetIO().InputQueueCharacters.Size; i++) {
        ImWchar ch = ImGui::GetIO().InputQueueCharacters[i];
        cLog::log (LOGINFO, fmt::format ("enter {:4x} {} {} {}",
                                         ch,altKeyPressed, ctrlKeyPressed, shiftKeyPressed));
        }
      ImGui::GetIO().InputQueueCharacters.resize (0);
      }
    //}}}

    // vars
    eTab mTab = eTelly;

    int64_t mMaxPidPackets = 0;
    size_t mPacketChars = 3;
    size_t mMaxNameChars = 3;
    size_t mMaxSidChars = 3;
    size_t mMaxPgmChars = 3;

    array <size_t,4> mPidMaxChars = { 3 };

    // !!! wrong !!! needed per service
    array <cTexture*,4> mSubtitleTextures = { nullptr };
    };
  //}}}
  }

// main
int main (int numArgs, char* args[]) {

  cTellyOptions* options = new cTellyOptions();

  // params
  cDvbMultiplex selectedMultiplex = kDvbMultiplexes[1];
  string filename;
  //{{{  parse commandLine params to options
  // parse params
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "all")
      options->mRecordAll = true;
    else if (param == "full")
      options->mFullScreen = true;
    else if (param == "simple")
      options->mShowAllServices = false;
    else if (param == "free")
      options->mVsync = false;
    else if (param == "simple")
      options->mShowAllServices = false;
    else if (param == "head") {
      options->mHeadless = true;
      options->mShowAllServices = false;
      }
    else if (param == "noaudio")
      options->mHasAudio = false;
    else if (param == "song")
      options->mPlaySong = true;
    else if (param == "sub")
      options->mShowSubtitle = true;
    else if (param == "motion")
      options->mHasMotionVectors = true;
    else if (param == "log1")
      options->mLogLevel = LOGINFO1;
    else if (param == "log2")
      options->mLogLevel = LOGINFO2;
    else if (param == "log3")
      options->mLogLevel = LOGINFO3;
    else {
      // assume filename
      filename = param;

      // look for named multiplex
      for (auto& multiplex : kDvbMultiplexes) {
        if (param == multiplex.mName) {
          // found named multiplex
          selectedMultiplex = multiplex;
          filename = "";
          break;
          }
        }
      }
    }

  selectedMultiplex.mRecordAll = options->mRecordAll;
  //}}}

  // log
  cLog::init (options->mLogLevel);
  cLog::log (LOGNOTICE, "tellyApp - all,simple,head,free,noaudio,song,full,sub,motion,log123,multiplexName,filename");

  if (options->mPlaySong) {
    cSongApp songApp (new cSongUI(), {800, 480}, options);
    songApp.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 20.f));
    songApp.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 20.f));
    songApp.setSongName (filename);
    songApp.mainUILoop();
    }
  else {
    cTellyApp tellyApp (new cTellyUI(), {1920/2, 1080/2}, options);
    if (!options->mHeadless) {
      tellyApp.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
      tellyApp.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 18.f));
      }
    if (filename.empty())
      tellyApp.liveDvbSource (selectedMultiplex, kRootDir, options->mShowAllServices);
    else
      tellyApp.fileSource (filename, options->mShowAllServices);
    tellyApp.mainUILoop();
    }

  return EXIT_SUCCESS;
  }

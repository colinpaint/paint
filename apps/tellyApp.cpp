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
#include "../common/basicTypes.h"
#include "../common/iOptions.h"
#include "../common/date.h"
#include "../common/utils.h"
#include "../common/fileUtils.h"
#include "../common/cLog.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/cGraphics.h"
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
#include "../decoders/cSubtitleFrame.h"
#include "../decoders/cSubtitleImage.h"
#include "../decoders/cFFmpegVideoDecoder.h"

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
  const vector <cDvbMultiplex> kDvbMultiplexes = {
      { "hd", 626000000,
        { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
        { "bbc1",          "bbc2",       "bbc3",         "bbc4",        "itv",     "c4",           "c5" },
        { 1,               2,            3,              4,             6,         7,              5 }
      },

      { "itv", 650000000,
        { "ITV1",   "ITV2",   "ITV3",   "ITV4",   "Channel 4", "Channel 4+1", "More 4",  "Film4" ,  "E4",   "Channel 5" },
        { "itv1sd", "itv2sd", "itv3sd", "itv4sd", "chn4sd",    "c4+1sd",      "more4sd", "film4sd", "e4sd", "chn5sd" },
        { 1,        2,        3,        4,        5,           6,             7,         8,         9,      0 }
      },

      { "bbc", 674000000,
        { "BBC ONE S West", "BBC TWO", "BBC FOUR" },
        { "bbc1sd",         "bbc2sd",  "bbc4sd" },
        { 1,                2,         4 }
      }
    };
  //}}}
  //{{{
  class cTellyOptions : public cApp::cOptions,
                        public cTransportStream::cOptions,
                        public cRender::cOptions,
                        public cAudioRender::cOptions,
                        public cVideoRender::cOptions,
                        public cFFmpegVideoDecoder::cOptions {
  public:
    virtual ~cTellyOptions() = default;

    string getString() const { return "song sub motion hd|bbc|itv filename"; }

    // vars
    bool mPlaySong = false;
    bool mShowEpg = true;
    bool mShowSubtitle = false;
    bool mShowMotionVectors = false;

    cDvbMultiplex mMultiplex = kDvbMultiplexes[0];
    string mFileName;
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
    cSongApp (cTellyOptions* options, iUI* ui) : cApp("songApp", options, ui) {
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
  class cSongUI : public cApp::iUI {
  public:
    virtual void draw (cApp& app, cGraphics& graphics) final {
      (void)graphics;

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
  //{{{  const string kRootDir
  #ifdef _WIN32
    const string kRootDir = "/tv/";
  #else
    const string kRootDir = "/home/pi/tv/";
  #endif
  //}}}
  //{{{
  class cTellyApp : public cApp {
  public:
    cTellyApp (cTellyOptions* options, iUI* ui) : cApp ("telly", options, ui), mOptions(options) {}
    virtual ~cTellyApp() = default;

    cTellyOptions* getOptions() { return mOptions; }
    bool hasTransportStream() { return mTransportStream != nullptr; }
    cTransportStream& getTransportStream() { return *mTransportStream; }

    // liveDvbSource
    bool isDvbSource() const { return mDvbSource; }
    cDvbSource& getDvbSource() { return *mDvbSource; }
    //{{{
    void liveDvbSource (const cDvbMultiplex& multiplex, cTellyOptions* options) {

      cLog::log (LOGINFO, fmt::format ("using multiplex {} {:4.1f} record {} {}",
                                       multiplex.mName, multiplex.mFrequency/1000.f,
                                       options->mRecordRoot, options->mShowAllServices ? "all " : ""));

      mFileSource = false;
      mMultiplex = multiplex;
      mRecordRoot = options->mRecordRoot;
      if (multiplex.mFrequency)
        mDvbSource = new cDvbSource (multiplex.mFrequency, 0);

      mTransportStream = new cTransportStream (multiplex, options);
      if (mTransportStream) {
        mLiveThread = thread ([=]() {
          cLog::setThreadName ("dvb ");

          FILE* mFile = options->mRecordAll ?
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
                if (options->mRecordAll && mFile)
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

            constexpr int kDvrReadChunkSize = 188 * 64;
            uint8_t* chunk = new uint8_t[kDvrReadChunkSize];

            uint64_t streamPos = 0;
            while (true) {
              int bytesRead = mDvbSource->getBlock (chunk, kDvrReadChunkSize);
              if (bytesRead) {
                streamPos += mTransportStream->demux (chunk, bytesRead, 0, false);
                if (options->mRecordAll && mFile)
                  fwrite (chunk, 1, bytesRead, mFile);
                }
              else
                cLog::log (LOGINFO, fmt::format ("liveDvbSource - no bytes"));
              }

            delete[] chunk;
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

    // fileSource
    bool isFileSource() { return mFileSource; }
    uint64_t getStreamPos() const { return mStreamPos; }
    uint64_t getFilePos() const { return mFilePos; }
    size_t getFileSize() const { return mFileSize; }
    std::string getFileName() const { return mOptions->mFileName; }
    //{{{
    void fileSource (const string& filename, cTellyOptions* options) {

      // create transportStream
      mTransportStream = new cTransportStream ({ "file", 0, {}, {}, {} }, options);
      if (!mTransportStream) {
        //{{{  error, return
        cLog::log (LOGERROR, "fileSource cTransportStream create failed");
        return;
        }
        //}}}

      // open fileSource
      mOptions->mFileName = cFileUtils::resolve (filename);
      FILE* file = fopen (mOptions->mFileName.c_str(), "rb");
      if (!file) {
        //{{{  error, return
        cLog::log (LOGERROR, fmt::format ("fileSource failed to open {}", mOptions->mFileName));
        return;
        }
        //}}}
      mFileSource = true;

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
    void hitControlLeft()  { skip (-90000 / 25); }
    void hitControlRight() { skip ( 90000 / 25); }
    void hitLeft()         { skip (-90000); }
    void hitRight()        { skip ( 90000); }
    void hitUp()           { skip (-900000); }
    void hitDown()         { skip ( 900000); }
    void hitSpace() { mTransportStream->togglePlay(); }
    void toggleShowEpg() { mOptions->mShowEpg = !mOptions->mShowEpg; }
    void toggleShowSubtitle() { mOptions->mShowSubtitle = !mOptions->mShowSubtitle; }
    void toggleShowMotionVectors() { mOptions->mShowMotionVectors = !mOptions->mShowMotionVectors; }

    //{{{
    void drop (const vector<string>& dropItems) {
    // drop fileSource

      for (auto& item : dropItems) {
        cLog::log (LOGINFO, fmt::format ("cTellyApp::drop {}", item));
        fileSource (item, mOptions);
        }
      }
    //}}}

  private:
    //{{{
    void skip (int64_t skipPts) {

      int64_t offset = mTransportStream->getSkipOffset (skipPts);
      cLog::log (LOGINFO, fmt::format ("skip:{} offset:{} pos:{}", skipPts, offset, mStreamPos));
      mStreamPos += offset;
      }
    //}}}

    cTellyOptions* mOptions;
    cTransportStream* mTransportStream = nullptr;

    // liveDvbSource
    thread mLiveThread;
    cDvbSource* mDvbSource = nullptr;
    cDvbMultiplex mMultiplex;
    string mRecordRoot;

    // fileSource
    bool mFileSource = false;
    FILE* mFile = nullptr;
    int64_t mStreamPos = 0;
    int64_t mFilePos = 0;
    size_t mFileSize = 0;
    };
  //}}}
  //{{{
  class cTellyUI : public cApp::iUI {
  public:
    //{{{
    virtual void draw (cApp& app, cGraphics& graphics) final {

      cTellyApp& tellyApp = (cTellyApp&)app;
      graphics.clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                       (int32_t)ImGui::GetIO().DisplaySize.y});

      ImGui::SetKeyboardFocusHere();
      ImGui::SetNextWindowPos ({0.f,0.f});
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
      ImGui::Begin ("telly", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar);

      if (tellyApp.hasTransportStream())
        mTab = (eTab)interlockedButtons (kTabNames, (uint8_t)mTab, {0.f,0.f}, true);

      //{{{  draw epg button
      ImGui::SameLine();
      if (toggleButton ("epg", tellyApp.getOptions()->mShowEpg))
        tellyApp.toggleShowEpg();
      //}}}
      //{{{  draw subtitle button
      ImGui::SameLine();
      if (toggleButton ("sub", tellyApp.getOptions()->mShowSubtitle))
        tellyApp.toggleShowSubtitle();
      //}}}
      //{{{  draw motionVectors button
      if (tellyApp.getOptions()->mHasMotionVectors) {
        ImGui::SameLine();
        if (toggleButton ("motion", tellyApp.getOptions()->mShowMotionVectors))
          tellyApp.toggleShowMotionVectors();
        }
      //}}}
      //{{{  draw fullScreen button
      if (tellyApp.getPlatform().hasFullScreen()) {
        ImGui::SameLine();
        if (toggleButton ("full", tellyApp.getPlatform().getFullScreen()))
          tellyApp.getPlatform().toggleFullScreen();
        }
      //}}}
      //{{{  draw vsync button
      ImGui::SameLine();
      if (tellyApp.getPlatform().hasVsync())
        if (toggleButton ("vsync", tellyApp.getPlatform().getVsync()))
          tellyApp.getPlatform().toggleVsync();
      //}}}
      //{{{  draw frameRate info
      ImGui::SameLine();
      ImGui::TextUnformatted(fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      //}}}

      if (tellyApp.hasTransportStream()) {
        cTransportStream& transportStream = tellyApp.getTransportStream();

        if (tellyApp.isFileSource()) {
          //{{{  draw streamPos info
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{:4.3f}%", tellyApp.getStreamPos() * 100.f /
                                                           tellyApp.getFileSize()).c_str());
          }
          //}}}
        else if (tellyApp.isDvbSource()) {
          //{{{  draw dvbSource signal,errors
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{} {}", tellyApp.getDvbSource().getTuneString(),
                                                        tellyApp.getDvbSource().getStatusString()).c_str());
          }
          //}}}

        if (tellyApp.getTransportStream().getNumErrors()) {
          //{{{  draw transportStream errors
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("error:{}", tellyApp.getTransportStream().getNumErrors()).c_str());
          }
          //}}}

        //{{{  draw tab subMenu with monoSpaced font
        ImGui::PushFont (tellyApp.getMonoFont());
        switch (mTab) {
          case ePids: drawPidMap (transportStream); break;
          case eRecord: drawRecordedFileNames (transportStream); break;
          default:;
          }
        ImGui::PopFont();
        //}}}

        if (transportStream.hasFirstTdt()) {
          //{{{  draw clock
          //ImGui::TextUnformatted (transportStream.getTdtString().c_str());
          ImGui::SetCursorPos ({ ImGui::GetWindowWidth() - 90.f, 0.f} );
          clockButton ("clock", transportStream.getNowTdt(), { 80.f, 80.f });
          }
          //}}}

        // draw piccies, strange order, after buttons displayed above ???
        mMultiView.draw (transportStream, graphics, tellyApp);
        }

      keyboard (tellyApp);
      ImGui::End();
      }
    //}}}

  private:
    //{{{
    class cMultiView {
    public:
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

      //{{{
      void draw (cTransportStream& transportStream, cGraphics& graphics, cTellyApp& tellyApp) {

        // create shaders, firsttime we see graphics interface
        if (!mVideoShader)
          mVideoShader = graphics.createTextureShader (cTexture::eYuv420);
        if (!mSubtitleShader)
          mSubtitleShader = graphics.createTextureShader (cTexture::eRgba);

        // update viewMap from enabled services, take care to reuse cView's
        for (auto& pair : transportStream.getServiceMap()) {
          cTransportStream::cService& service = pair.second;

          // find service sid in viewMap
          auto it = mViewMap.find (pair.first);
          if (it == mViewMap.end()) {
            if (service.getStream (cTransportStream::eVideo).isDefined())
              // enabled and not found, add service to viewMap
              mViewMap.emplace (service.getSid(), cView (service));
            }
          //else if (!service.getStream (eVideo).isEnabled())
            // found, but not enabled, remove service from viewMap
           // mViewMap.erase (it);
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
            if (view.second.draw (transportStream, graphics, tellyApp,
                                  selectedFull, selectedFull ? 1 : mViewMap.size(), viewIndex,
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

    private:
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
        bool draw (cTransportStream& transportStream, cGraphics& graphics, cTellyApp& tellyApp,
                   bool selectFull, size_t numViews, size_t viewIndex,
                   cTextureShader* videoShader, cTextureShader* subtitleShader) {
        // return true if hit

          cTellyOptions* options = tellyApp.getOptions();

          float layoutScale;
          cVec2 layoutPos = getLayout (viewIndex, numViews, layoutScale);
          float viewportWidth = ImGui::GetWindowWidth();
          float viewportHeight = ImGui::GetWindowHeight();
          mTl = {(layoutPos.x - (layoutScale/2.f)) * viewportWidth,
                 (layoutPos.y - (layoutScale/2.f)) * viewportHeight};
          mBr = {(layoutPos.x + (layoutScale/2.f)) * viewportWidth,
                 (layoutPos.y + (layoutScale/2.f)) * viewportHeight};

          bool enabled = mService.getStream (cTransportStream::eVideo).isEnabled();
          if (enabled) {
            int64_t playPts = mService.getStream (cTransportStream::eAudio).getRender().getPts();
            if (mService.getStream (cTransportStream::eAudio).isEnabled()) {
              // get playPts from audioStream
              cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (cTransportStream::eAudio).getRender());
              if (audioRender.getPlayer())
                playPts = audioRender.getPlayer()->getPts();
              }
            if (!selectFull || (mSelect != eUnselected)) {
              //{{{  view selected or no views selected, show nearest videoFrame to playPts
              cVideoRender& videoRender = dynamic_cast<cVideoRender&>(mService.getStream (cTransportStream::eVideo).getRender());
              cVideoFrame* videoFrame = videoRender.getVideoFrameAtOrAfterPts (playPts);
              if (videoFrame) {
                //{{{  draw video
                cMat4x4 model = cMat4x4();
                model.setTranslate ({(layoutPos.x - (0.5f * layoutScale)) * viewportWidth,
                                     ((1.f-layoutPos.y) - (0.5f * layoutScale)) * viewportHeight});
                model.size ({layoutScale * viewportWidth / videoFrame->getWidth(),
                             layoutScale * viewportHeight / videoFrame->getHeight()});
                cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);
                videoShader->use();
                videoShader->setModelProjection (model, projection);

                // texture
                cTexture& texture = videoFrame->getTexture (graphics);
                texture.setSource();

                // ensure quad is created
                if (!mVideoQuad)
                  mVideoQuad = graphics.createQuad (videoFrame->getSize());

                // draw quad
                mVideoQuad->draw();
                //}}}
                if (options->mShowSubtitle) {
                  //{{{  draw subtitles
                  cSubtitleRender& subtitleRender =
                    dynamic_cast<cSubtitleRender&> (mService.getStream (cTransportStream::eSubtitle).getRender());

                  subtitleShader->use();
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
                    model.setTranslate ({(layoutPos.x + ((xpos - 0.5f) * layoutScale)) * viewportWidth,
                                         ((1.0f - layoutPos.y) + ((ypos - 0.5f) * layoutScale)) * viewportHeight});
                    subtitleShader->setModelProjection (model, projection);

                    // ensure quad is created (assumes same size) and draw
                    if (!mSubtitleQuads[line])
                      mSubtitleQuads[line] = graphics.createQuad (mSubtitleTextures[line]->getSize());
                    mSubtitleQuads[line]->draw();
                    }
                  }
                  //}}}
                if (options->mShowMotionVectors && (mSelect == eSelectedFull)) {
                  //{{{  draw motion vectors
                  size_t numMotionVectors;
                  AVMotionVector* mv = (AVMotionVector*)(videoFrame->getMotionVectors (numMotionVectors));
                  if (numMotionVectors) {
                    for (size_t i = 0; i < numMotionVectors; i++) {
                      ImGui::GetWindowDrawList()->AddLine (
                        mTl + ImVec2(mv->src_x * viewportWidth / videoFrame->getWidth(),
                                     mv->src_y * viewportHeight / videoFrame->getHeight()),
                        mTl + ImVec2((mv->src_x + (mv->motion_x / mv->motion_scale)) * viewportWidth / videoFrame->getWidth(),
                                      (mv->src_y + (mv->motion_y / mv->motion_scale)) * viewportHeight / videoFrame->getHeight()),
                        mv->source > 0 ? 0xc0c0c0c0 : 0xc000c0c0, 1.f);
                      mv++;
                      }
                    }
                  }
                  //}}}
                }

              if (mService.getStream (cTransportStream::eAudio).isEnabled()) {
                cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getStream (cTransportStream::eAudio).getRender());
                if (audioRender.getPlayer())
                  audioRender.getPlayer()->setMute (mSelect == eUnselected);

                // draw audioMeter graphic
                mAudioMeterView.draw (audioRender, playPts,
                                      ImVec2(mBr.x - (0.5f * ImGui::GetTextLineHeight()),
                                             mBr.y - (0.5f * ImGui::GetTextLineHeight())));
                // draw frames graphic
                if (mSelect == eSelectedFull)
                  mFramesView.draw (audioRender, videoRender, playPts,
                                    ImVec2((mTl.x + mBr.x)/2.f,
                                           mBr.y - (0.5f * ImGui::GetTextLineHeight())));
                }
              }
              //}}}
            }

          if ((mSelect == eSelectedFull) && (options->mShowEpg)) {
            //{{{  draw epg
            auto nowDatePoint = date::floor<date::days>(transportStream.getNowTdt());

            ImVec2 pos = mTl + ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()*2);
            for (auto& epgItem : mService.getEpgItemMap()) {
              auto startTime = epgItem.second->getTime();
              if ((startTime > transportStream.getNowTdt()) &&
                  (date::floor<date::days>(startTime) == nowDatePoint)) {
                string epg = date::format ("%T", date::floor<chrono::seconds>(startTime)) +
                             " " + epgItem.second->getTitleString();

                ImGui::SetCursorPos (pos);
                ImGui::TextColored ({0.f,0.f,0.f,1.f}, epg.c_str());
                ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
                ImGui::TextColored ({1.f, 1.f,1.f,1.f}, epg.c_str());

                pos.y += ImGui::GetTextLineHeight();
                }
              }
            }
            //}}}

          //{{{  draw channel title bottomLeft
          string channelString = mService.getName();
          if (!enabled || (mSelect == eSelectedFull))
            channelString += " " + mService.getNowTitleString();

          ImVec2 pos = {mTl.x + (ImGui::GetTextLineHeight() * 0.25f),
                        mBr.y - ImGui::GetTextLineHeight()};
          ImGui::SetCursorPos (pos);
          ImGui::TextColored ({0.f,0.f,0.f,1.f}, channelString.c_str());
          ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
          ImGui::TextColored ({1.f, 1.f,1.f,1.f}, channelString.c_str());
          //}}}
          //{{{  draw ptsFromStart bottom right
          string ptsFromStartString = utils::getPtsString (mService.getPtsFromStart());

          pos = mBr - ImVec2(ImGui::GetTextLineHeight() * 6.f, ImGui::GetTextLineHeight());
          ImGui::SetCursorPos (pos);
          ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsFromStartString.c_str());
          ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
          ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsFromStartString.c_str());
          //}}}

          // draw select rect
          bool hover = ImGui::IsMouseHoveringRect (mTl, mBr);
          if ((hover || (mSelect != eUnselected)) && (mSelect != eSelectedFull))
            ImGui::GetWindowDrawList()->AddRect (mTl, mBr, hover ? 0xff20ffff : 0xff20ff20, 4.f, 0, 4.f);

          ImGui::SetCursorPos (mTl);
          if (ImGui::InvisibleButton (fmt::format ("view##{}", mService.getSid()).c_str(), mBr-mTl)) {
            // hit view, select action
            if (!mService.getStream (cTransportStream::eVideo).isEnabled())
              mService.enableStreams();
            else if (mSelect == eUnselected)
              mSelect = eSelected;
            else if (mSelect == eSelected)
              mSelect = eSelectedFull;
            else if (mSelect == eSelectedFull)
              mSelect = eSelected;
            return true;
            }

          return false;
          }
        //}}}

      private:
        static const size_t kMaxSubtitleLines = 4;
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
        ImVec2 mTl;
        ImVec2 mBr;
        cQuad* mVideoQuad = nullptr;

        // graphics
        cFramesView mFramesView;
        cAudioMeterView mAudioMeterView;

        // subtitle
        array <cQuad*, kMaxSubtitleLines> mSubtitleQuads = { nullptr };
        array <cTexture*, kMaxSubtitleLines> mSubtitleTextures = { nullptr };
        };
      //}}}
      map <uint16_t, cView> mViewMap;

      cTextureShader* mVideoShader = nullptr;
      cTextureShader* mSubtitleShader = nullptr;
      };
    //}}}

    enum eTab { eTelly, ePids, eRecord };
    inline static const vector<string> kTabNames = { "telly", "pids", "recorded" };

    //{{{
    void hitShow (eTab tab) {
      mTab = (tab == mTab) ? eTelly : tab;
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
                                pidInfo.getPidName()).c_str());

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
    void drawRecordedFileNames (cTransportStream& transportStream) {

      for (auto& program : transportStream.getRecordedFileNames())
        ImGui::TextUnformatted (program.c_str());
      }
    //}}}

    //{{{
    void hitSpace (cTellyApp& tellyApp) {

      if (tellyApp.isFileSource())
        tellyApp.hitSpace();
      else
        mMultiView.hitSpace();
      }
    //}}}
    //{{{
    void hitControlLeft (cTellyApp& tellyApp) {

      if (tellyApp.isFileSource())
        tellyApp.hitControlLeft();
      }
    //}}}
    //{{{
    void hitControlRight (cTellyApp& tellyApp) {

      if (tellyApp.isFileSource())
        tellyApp.hitControlRight();
      }
    //}}}
    //{{{
    void hitLeft (cTellyApp& tellyApp) {

      if (tellyApp.isFileSource())
        tellyApp.hitLeft();
      else
        mMultiView.hitLeft();
      }
    //}}}
    //{{{
    void hitRight (cTellyApp& tellyApp) {

      if (tellyApp.isFileSource())
        tellyApp.hitRight();
      else
        mMultiView.hitRight();
      }
    //}}}
    //{{{
    void hitUp (cTellyApp& tellyApp) {

      if (tellyApp.isFileSource())
        tellyApp.hitUp();
      else
        mMultiView.hitUp();
      }
    //}}}
    //{{{
    void hitDown (cTellyApp& tellyApp) {

      if (tellyApp.isFileSource())
        tellyApp.hitDown();
      else
        mMultiView.hitDown();
      }
    //}}}
    //{{{
    void keyboard (cTellyApp& tellyApp) {

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
        { false, false,  false, ImGuiKey_Space,      [this,&tellyApp]{ hitSpace (tellyApp); }},
        { false, true,   false, ImGuiKey_LeftArrow,  [this,&tellyApp]{ hitControlLeft (tellyApp); }},
        { false, true,   false, ImGuiKey_RightArrow, [this,&tellyApp]{ hitControlRight (tellyApp); }},
        { false, false,  false, ImGuiKey_LeftArrow,  [this,&tellyApp]{ hitLeft (tellyApp); }},
        { false, false,  false, ImGuiKey_RightArrow, [this,&tellyApp]{ hitRight (tellyApp); }},
        { false, false,  false, ImGuiKey_UpArrow,    [this,&tellyApp]{ hitUp (tellyApp); }},
        { false, false,  false, ImGuiKey_DownArrow,  [this,&tellyApp]{ hitDown (tellyApp); }},
        { false, false,  false, ImGuiKey_Enter,      [this,&tellyApp]{ mMultiView.hitEnter(); }},
        { false, false,  false, ImGuiKey_F,          [this,&tellyApp]{ tellyApp.getPlatform().toggleFullScreen(); }},
        { false, false,  false, ImGuiKey_E,          [this,&tellyApp]{ tellyApp.toggleShowEpg(); }},
        { false, false,  false, ImGuiKey_S,          [this,&tellyApp]{ tellyApp.toggleShowSubtitle(); }},
        { false, false,  false, ImGuiKey_L,          [this,&tellyApp]{ tellyApp.toggleShowMotionVectors(); }},
        { false, false,  false, ImGuiKey_T,          [this,&tellyApp]{ hitShow (eTelly); }},
        { false, false,  false, ImGuiKey_P,          [this,&tellyApp]{ hitShow (ePids); }},
        { false, false,  false, ImGuiKey_R,          [this,&tellyApp]{ hitShow (eRecord); }},
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

      // queued keys
      //for (int i = 0; i < ImGui::GetIO().InputQueueCharacters.Size; i++) {
      //  ImWchar ch = ImGui::GetIO().InputQueueCharacters[i];
      //  cLog::log (LOGINFO, fmt::format ("enter {:4x} {} {} {}", ch, altKey, controlKey, shiftKey));
      //  }
      ImGui::GetIO().InputQueueCharacters.resize (0);
      }
    //}}}

    // vars
    eTab mTab = eTelly;
    cMultiView mMultiView;

    // pidInfo tabs
    int64_t mMaxPidPackets = 0;
    size_t mPacketChars = 3;
    size_t mMaxNameChars = 3;
    size_t mMaxSidChars = 3;
    size_t mMaxPgmChars = 3;
    };
  //}}}
  }

// main
int main (int numArgs, char* args[]) {

  cTellyOptions* options = new cTellyOptions();
  //{{{  parse commandLine params to options
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) // found cApp option
      ;
    else if (param == "all")
      options->mRecordAll = true;
    else if (param == "head") {
      options->mHasGui = false;
      options->mShowAllServices = false;
      options->mShowFirstService = false;
      }
    else if (param == "single") {
      options->mShowAllServices = false;
      options->mShowFirstService = true;
      }
    else if (param == "noaudio")
      options->mHasAudio = false;
    else if (param == "song")
      options->mPlaySong = true;
    else if (param == "sub")
      options->mShowSubtitle = true;
    else if (param == "motion")
      options->mHasMotionVectors = true;
    else {
      // assume filename
      options->mFileName = param;

      // look for named multiplex
      for (auto& multiplex : kDvbMultiplexes) {
        if (param == multiplex.mName) {
          // found named multiplex
          options->mMultiplex = multiplex;
          options->mFileName = "";
          break;
          }
        }
      }
    }
  //}}}

  // log
  cLog::init (options->mLogLevel);
  cLog::log (LOGNOTICE, fmt::format ("tellyApp head all single noaudio {} {}",
                                     (dynamic_cast<cApp::cOptions*>(options))->cApp::cOptions::getString(),
                                     options->cTellyOptions::getString()));
  if (options->mPlaySong) {
    cSongApp songApp (options, new cSongUI());
    songApp.setSongName (options->mFileName);
    songApp.mainUILoop();
    return EXIT_SUCCESS;
    }

  cTellyApp tellyApp (options, new cTellyUI());
  if (options->mFileName.empty()) {
    // liveDvb source
    options->mIsLive = true;
    options->mRecordRoot = kRootDir;
    tellyApp.liveDvbSource (options->mMultiplex, options);
    }
  else // file source
    tellyApp.fileSource (options->mFileName, options);

  tellyApp.mainUILoop();
  return EXIT_SUCCESS;
  }

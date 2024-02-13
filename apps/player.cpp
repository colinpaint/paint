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
#include <mutex>
#include <shared_mutex>

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
#include "../dvb/cTransportStream.h"
#include "../dvb/cRender.h"
#include "../dvb/cVideoRender.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cPlayer.h"

using namespace std;
//}}}

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
//{{{
class cFilePlayer {
public:
  cFilePlayer (string fileName, cPlayerOptions* options) :
    mFileName(cFileUtils::resolve (fileName)), mOptions(options) {}
  //{{{
  //virtual ~cFilePlayer() {
    //mVideoPesMap.clear();
    //mAudioPesMap.clear();
    //mSubtitlePesMap.clear();
    //}
  //}}}

  string getFileName() const { return mFileName; }
  cTransportStream::cService* getService() { return mService; }

  int64_t getFirstVideoPts() const { return mVideoPesMap.empty() ? -1 : mVideoPesMap.begin()->first; }
  int64_t getFirstAudioPts() const { return mAudioPesMap.empty() ? -1 : mAudioPesMap.begin()->first; }

  cAudioRender* getAudioRender() { return (cAudioRender*)(mAudioRender); }
  cVideoRender* getVideoRender() { return (cVideoRender*)(mVideoRender); }
  //{{{
  bool analyse() {
  // launch analyser thread

    FILE* file = fopen (mFileName.c_str(), "rb");
    if (!file) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("cFilePlayer::analyse to open {}", mFileName));
      return false;
      }
      //}}}

    // get fileSize
    #ifdef _WIN32
      struct _stati64 st;
       if (_stat64 (mFileName.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #else
      struct stat st;
      if (stat (mFileName.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #endif

    cTransportStream* transportStream = new cTransportStream (
      {"anal", 0, {}, {}}, mOptions,
      //{{{  newService lambda
      [&](cTransportStream::cService& service) noexcept {
        if (!mService) {
          mService = &service;
          mAudioRender = new cAudioRender ("aud", mService->getAudioStreamTypeId(),
                                                  mService->getAudioPid(), mOptions);
          mVideoRender = new cVideoRender ("vid", mService->getVideoStreamTypeId(),
                                                  mService->getVideoPid(), mOptions);
          mSubtitleRender = new cSubtitleRender ("sub", mService->getSubtitleStreamTypeId(),
                                                        mService->getSubtitlePid(), mOptions);
          }
        },
      //}}}
      //{{{  pes lambda
      [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo, bool skip) noexcept {

        (void)skip;
        mPesBytes += pidInfo.getBufSize();
        uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
        memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());

        //unique_lock<shared_mutex> lock (mAudioMutex);

        // add pes to maps
        if (pidInfo.getPid() == service.getVideoPid())
          mVideoPesMap.emplace (pidInfo.getPts(), cPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
        else if (pidInfo.getPid() == service.getAudioPid())
          mAudioPesMap.emplace (pidInfo.getPts(), cPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
        else if (pidInfo.getPid() == service.getSubtitlePid())
          if (pidInfo.getBufSize())
            mSubtitlePesMap.emplace (pidInfo.getPts(), cPes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), pidInfo.getDts()));
        }
      //}}}
      );
    if (!transportStream) {
      //{{{  error, return
      cLog::log (LOGERROR, "cFilePlayer::analyser ts create failed");
      return false;
      }
      //}}}

    thread ([=]() {
      cLog::setThreadName ("anal");
      size_t chunkSize = 188 * 256;
      uint8_t* chunk = new uint8_t[chunkSize];
      auto now = chrono::system_clock::now();
      int64_t streamPos = 0;
      while (true) {
        size_t bytesRead = fread (chunk, 1, chunkSize, file);
        if (bytesRead > 0)
          streamPos += transportStream->demux (chunk, bytesRead, streamPos, false);
        else
          break;
        }
      delete[] chunk;
      fclose (file);
      delete transportStream;
      //{{{  log totals
      cLog::log (LOGINFO, fmt::format ("size:{:8d}:{:8d} took {}ms",
        mPesBytes, mFileSize,
        chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count()));
      cLog::log (LOGINFO, fmt::format ("- vid:{:6d} {} to {}",
                                       mVideoPesMap.size(),
                                       utils::getFullPtsString (mVideoPesMap.begin()->first),
                                       utils::getFullPtsString (mVideoPesMap.rbegin()->first)));
      cLog::log (LOGINFO, fmt::format ("- aud:{:6d} {} to {}",
                                       mAudioPesMap.size(),
                                       utils::getFullPtsString (mAudioPesMap.begin()->first),
                                       utils::getFullPtsString (mAudioPesMap.rbegin()->first)));
      cLog::log (LOGINFO, fmt::format ("- sub:{:6d} {} to {}",
                                       mSubtitlePesMap.size(),
                                       utils::getFullPtsString (mSubtitlePesMap.begin()->first),
                                       utils::getFullPtsString (mSubtitlePesMap.rbegin()->first)));
      cLog::log (LOGERROR, "exit");
      //}}}
      }).detach();

    return true;
    }
  //}}}
  //{{{
  void loader() {
  // launch loader thread

    thread ([=]() {
      cLog::setThreadName ("load");

      // wait to get going
      this_thread::sleep_for (100ms);

      for (auto& pair : mAudioPesMap) {
        cLog::log (LOGINFO, fmt::format ("- load audio pes {}", utils::getFullPtsString (pair.first)));
        mAudioRender->decodePes (pair.second.mData, pair.second.mSize,
                                 0, pair.second.mPts, pair.second.mDts, false);
        this_thread::sleep_for (21ms);
        }

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
  //}}}

  //{{{
  void togglePlay() {
    if (mAudioRender)
      mAudioRender->togglePlay();
    }
  //}}}
  //{{{
  void skip (int64_t skipPts) {
    if (mService)
      mService->skip (skipPts);
    }
  //}}}

  shared_mutex mAudioMutex;

private:
  //{{{
  class cPes {
  public:
    cPes (uint8_t* data, uint32_t size, int64_t pts, int64_t dts) :
      mData(data), mSize(size), mPts(pts), mDts(dts) {}
    ~cPes() = default;

    uint8_t* mData;
    uint32_t mSize;
    int64_t mPts;
    int64_t mDts;
    };
  //}}}

  string mFileName;
  cPlayerOptions* mOptions;

  cTransportStream::cService* mService = nullptr;

  size_t mFileSize = 0;
  int64_t mPesBytes = 0;

  map <int64_t,cPes> mAudioPesMap;
  map <int64_t,cPes> mVideoPesMap;
  map <int64_t,cPes> mSubtitlePesMap;

  cRender* mVideoRender = nullptr;
  cRender* mAudioRender = nullptr;
  cRender* mSubtitleRender = nullptr;
  };
//}}}
//{{{
class cPlayerApp : public cApp {
public:
  cPlayerApp (cPlayerOptions* options, iUI* ui) : cApp ("Player", options, ui), mOptions(options) {}
  //{{{
  virtual ~cPlayerApp() {
    // delete filePlayes
    mFilePlayers.clear();
    }
  //}}}

  //{{{
  void addFile (const string& fileName, cPlayerOptions* options) {

    cFilePlayer* filePlayer = new cFilePlayer (fileName, options);

    mFilePlayers.push_back (filePlayer);
    filePlayer->analyse();
    filePlayer->loader();
    }
  //}}}

  cPlayerOptions* getOptions() { return mOptions; }
  cFilePlayer* getFilePlayer() { return mFilePlayers.front(); }

  // actions
  void toggleShowSubtitle() { mOptions->mShowSubtitle = !mOptions->mShowSubtitle; }
  void toggleShowMotionVectors() { mOptions->mShowMotionVectors = !mOptions->mShowMotionVectors; }

  // play
  //{{{
  void togglePlay() {
    mFilePlayers.front()->togglePlay();
    }
  //}}}
  //{{{
  void skipPlay (int64_t skipPts) {
    cLog::log (LOGINFO, fmt::format ("skip:{}", skipPts));
    mFilePlayers.front()->skip (skipPts);
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
  vector<cFilePlayer*> mFilePlayers;
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

    // draw multiView piccies
    mMultiView.draw (playerApp);

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
    bool draw (cPlayerApp& playerApp, bool selectFull, size_t viewIndex, size_t numViews,
               cTextureShader* videoShader, cTextureShader* subtitleShader) {
    // return true if hit

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

      cAudioRender* audioRender = playerApp.getFilePlayer()->getAudioRender();
      if (audioRender) {
        //  get audio playPts
        int64_t playPts = audioRender->getPts();
        if (audioRender->getPlayer()) {
          playPts = audioRender->getPlayer()->getPts();
          audioRender->getPlayer()->setMute (false);
          }

        cVideoRender* videoRender = playerApp.getFilePlayer()->getVideoRender();
        if (videoRender) {
          cVideoFrame* videoFrame = videoRender->getVideoFrameAtOrAfterPts (playPts);
          if (videoFrame) {
            //{{{
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

            //if (playerApp.getOptions()->mShowSubtitle) {
              //{{{  draw subtitles
              //cSubtitleRender& subtitleRender =
                //dynamic_cast<cSubtitleRender&> (mService.getStream (cRenderStream::eSubtitle).getRender());

              //subtitleShader->use();
              //for (size_t line = 0; line < subtitleRender.getNumLines(); line++) {
                //cSubtitleImage& subtitleImage = subtitleRender.getImage (line);
                //if (!mSubtitleTextures[line])
                  //mSubtitleTextures[line] = playerApp.getGraphics().createTexture (cTexture::eRgba, subtitleImage.getSize());
                //mSubtitleTextures[line]->setSource();

                //// update subtitle texture if image dirty
                //if (subtitleImage.isDirty())
                  //mSubtitleTextures[line]->setPixels (subtitleImage.getPixels(), nullptr);
                //subtitleImage.setDirty (false);

                //float xpos = (float)subtitleImage.getXpos() / videoFrame->getWidth();
                //float ypos = (float)(videoFrame->getHeight() - subtitleImage.getYpos()) / videoFrame->getHeight();
                //model.setTranslate ({(layoutPos.x + ((xpos - 0.5f) * layoutScale)) * viewportWidth,
                                     //((1.0f - layoutPos.y) + ((ypos - 0.5f) * layoutScale)) * viewportHeight});
                //subtitleShader->setModelProjection (model, projection);

                //// ensure quad is created (assumes same size) and draw
                //if (!mSubtitleQuads[line])
                  //mSubtitleQuads[line] = playerApp.getGraphics().createQuad (mSubtitleTextures[line]->getSize());
                //mSubtitleQuads[line]->draw();
                //}
              //}
              //}}}

            //if (playerApp.getOptions()->mShowMotionVectors && (mSelect == eSelectedFull)) {
              //{{{  draw motion vectors
              //size_t numMotionVectors;
              //AVMotionVector* mv = (AVMotionVector*)(videoFrame->getMotionVectors (numMotionVectors));
              //if (numMotionVectors) {
                //for (size_t i = 0; i < numMotionVectors; i++) {
                  //ImGui::GetWindowDrawList()->AddLine (
                    //mTL + ImVec2(mv->src_x * viewportWidth / videoFrame->getWidth(),
                                 //mv->src_y * viewportHeight / videoFrame->getHeight()),
                    //mTL + ImVec2((mv->src_x + (mv->motion_x / mv->motion_scale)) * viewportWidth / videoFrame->getWidth(),
                                  //(mv->src_y + (mv->motion_y / mv->motion_scale)) * viewportHeight / videoFrame->getHeight()),
                    //mv->source > 0 ? 0xc0c0c0c0 : 0xc000c0c0, 1.f);
                  //mv++;
                  //}
                //}
              //}
              //}}}
            }
            //}}}

          if (mSelect == eSelectedFull) // draw framesView
            mFramesView.draw (*audioRender, *videoRender, playPts,
                              ImVec2((mTL.x + mBR.x)/2.f, mBR.y - ImGui::GetTextLineHeight()*0.25f));
          }

        if (audioRender) // draw audioMeterView
          mAudioMeterView.draw (*audioRender, playPts,
                                ImVec2(mBR.x - ImGui::GetTextLineHeight()*0.5f,
                                       mBR.y - ImGui::GetTextLineHeight()*0.25f));

        // largeFont for title and pts
        ImGui::PushFont (playerApp.getLargeFont());
        string title = "title";
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
        }

      ImVec2 viewSubSize = mSize - ImVec2(0.f,
          ImGui::GetTextLineHeight() * ((layoutPos.y + (layoutScale/2.f) >= 0.99f) ? 3.f : 1.5f));

      // invisbleButton over view sub area
      ImGui::SetCursorPos ({0.f,0.f});
      if (ImGui::InvisibleButton (fmt::format ("viewBox##{}", mService.getSid()).c_str(), viewSubSize)) {
        //{{{  hit view, select action
        if (mSelect == eUnselected)
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
    void draw (cPlayerApp& playerApp) {

      // create shaders, firsttime we see graphics interface
      if (!mVideoShader)
        mVideoShader = playerApp.getGraphics().createTextureShader (cTexture::eYuv420);
      if (!mSubtitleShader)
        mSubtitleShader = playerApp.getGraphics().createTextureShader (cTexture::eRgba);

      // update viewMap from filePlayers
      cFilePlayer* filePlayer = playerApp.getFilePlayer();

      auto it = mViewMap.find (filePlayer->getService()->getSid());
      if (it == mViewMap.end()) {
        mViewMap.emplace (filePlayer->getService()->getSid(), cView (*filePlayer->getService()));
        cLog::log (LOGINFO, fmt::format ("cMultiView::draw add view {}", filePlayer->getService()->getSid()));
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
          if (view.second.draw (playerApp, selectedFull, viewIndex, selectedFull ? 1 : mViewMap.size(),
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
    playerApp.skipPlay (-90000 / 25);
    }
  //}}}
  //{{{
  void hitControlRight (cPlayerApp& playerApp) {
    playerApp.skipPlay (90000 / 25);
    }
  //}}}
  //{{{
  void hitLeft (cPlayerApp& playerApp) {
    playerApp.skipPlay (-90000);
    }
  //}}}
  //{{{
  void hitRight (cPlayerApp& playerApp) {
    playerApp.skipPlay (90000 / 25);
    }
  //}}}
  //{{{
  void hitUp (cPlayerApp& playerApp) {
    playerApp.skipPlay (-900000 / 25);
    }
  //}}}
  //{{{
  void hitDown (cPlayerApp& playerApp) {
    playerApp.skipPlay (900000 / 25);
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

  // launch
  cPlayerApp playerApp (options, new cPlayerUI());
  playerApp.addFile (options->mFileName, options);
  playerApp.mainUILoop();

  return EXIT_SUCCESS;
  }

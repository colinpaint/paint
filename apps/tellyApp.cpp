// tellyApp.cpp - imgui tellyApp, main, UI
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>

//#ifdef __linux__
//  #include <unistd.h>
//  #include <sys/poll.h>
//#endif

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
#include "../app/cUI.h"
#include "../font/itcSymbolBold.h"
#include "../font/droidSansMono.h"

// dvb
#include "../dvb/cDvbSource.h"
#include "../dvb/cTransportStream.h"
#include "../dvb/cVideoRender.h"
#include "../dvb/cVideoFrame.h"
#include "../dvb/cAudioRender.h"
#include "../dvb/cAudioFrame.h"
#include "../dvb/cSubtitleRender.h"
#include "../dvb/cSubtitleImage.h"
#include "../dvb/cPlayer.h"

using namespace std;
//}}}
namespace {
  //{{{
  const vector <cDvbMultiplex> kMultiplexes = {
      { "file", false, 0, {}, {} },  // dummy multiplex for file

      { "hd", 626000000, false,
        { "BBC ONE SW HD", "BBC TWO HD", "BBC THREE HD", "BBC FOUR HD", "ITV1 HD", "Channel 4 HD", "Channel 5 HD" },
        { "bbc1hd",        "bbc2hd",     "bbc3hd",       "bbc4hd",      "itv1hd",  "chn4hd",       "chn5hd" }
      },

      { "itv", 650000000, false,
        { "ITV1",  "ITV2", "ITV3", "ITV4", "Channel 4", "Channel 4+1", "More 4", "Film4" , "E4", "Channel 5" },
        { "itv1", "itv2", "itv3", "itv4", "chn4"     , "c4+1",        "more4",  "film4",  "e4", "chn5" }
      },

      { "bbc", 674000000, false,
        { "BBC ONE S West", "BBC TWO", "BBC FOUR" },
        { "bbc1",           "bbc2",    "bbc4" }
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
  class cServiceView {
  public:
    cServiceView (cTransportStream::cService& service) : mService(service) {}
    ~cServiceView() = default;

    bool getHover() const { return mHover; }

    bool picked (cVec2 pos) { return mRect.isInside (pos); }
    //{{{
    bool hover() {
      mHover = ImGui::IsMouseHoveringRect (ImVec2 ((float)mRect.left, (float)mRect.top),
                                           ImVec2 ((float)mRect.right, (float)mRect.bottom));
      return mHover;
      }
    //}}}

    //{{{
    void trim() {

      cVideoRender& videoRender = dynamic_cast<cVideoRender&> (mService.getRenderStream (eRenderVideo).getRender());

      int64_t playerPts = mService.getRenderStream (eRenderAudio).getPts();
      if (mService.getRenderStream (eRenderAudio).isEnabled()) {
        // get playerPts from audioStream
        cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getRenderStream (eRenderAudio).getRender());
        playerPts = audioRender.getPlayer().getPts();
        }

      videoRender.trimVideoBeforePts (playerPts - videoRender.getPtsDuration());
      }
    //}}}

    //{{{
    void draw (cGraphics& graphics, bool selected, size_t numViews, size_t viewIndex) {

      float scale = (numViews <= 1) ? 1.f : ((numViews <= 4) ? 0.5f : ((numViews <= 9) ? 0.33f : 0.25f));
      float viewportWidth = ImGui::GetWindowWidth();
      float viewportHeight = ImGui::GetWindowHeight();
      cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);

      cVideoRender& videoRender = dynamic_cast<cVideoRender&> (mService.getRenderStream (eRenderVideo).getRender());

      int64_t playerPts = mService.getRenderStream (eRenderAudio).getPts();
      if (mService.getRenderStream (eRenderAudio).isEnabled()) {
        //  get playerPts from audioStream, draw framesView, audioMeter
        cAudioRender& audioRender = dynamic_cast<cAudioRender&>(mService.getRenderStream (eRenderAudio).getRender());
        playerPts = audioRender.getPlayer().getPts();

        // draw audio meter
        mAudioMeterView.draw (audioRender, playerPts,
                              ImVec2((float)mRect.right - (0.25f * ImGui::GetTextLineHeight()),
                                     (float)mRect.bottom - (0.25f * ImGui::GetTextLineHeight())));

        // listen and draw framesView if selected
        audioRender.getPlayer().setMute (!selected);
        if (selected)
          mFramesView.draw (audioRender, videoRender, playerPts,
                            ImVec2((float)mRect.getCentre().x,
                                   (float)mRect.bottom - (0.25f * ImGui::GetTextLineHeight())));
        }

      // get playerPts nearest videoFrame
      cVideoFrame* videoFrame = videoRender.getVideoNearestFrameFromPts (playerPts);
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
        //}}}

        mRect = cRect (mModel.transform (cVec2(0, videoFrame->getHeight()), viewportHeight),
                       mModel.transform (cVec2(videoFrame->getWidth(), 0), viewportHeight));
        if (mHover)
          ImGui::GetWindowDrawList()->AddRect ({ (float)mRect.left, (float)mRect.top},
                                               { (float)mRect.right, (float)mRect.bottom },
                                               0xffc0ffff);

        cSubtitleRender& subtitleRender = dynamic_cast<cSubtitleRender&> (mService.getRenderStream (eRenderSubtitle).getRender());
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

        // crude management of videoFrame cache
        videoRender.trimVideoBeforePts (playerPts - videoFrame->mPtsDuration);
        }
      }
    //}}}

    inline static cTextureShader* mVideoShader = nullptr;
    inline static cTextureShader* mSubtitleShader = nullptr;

  private:
    //{{{
    class cFramesView {
    public:
      cFramesView() {}
      ~cFramesView() = default;

      //{{{
      void draw (cAudioRender& audioRender, cVideoRender& videoRender, int64_t playerPts, ImVec2 pos) {

        // draw playPts centre bar
        ImGui::GetWindowDrawList()->AddRectFilled (
          { pos.x, pos.y - (kLines * ImGui::GetTextLineHeight()) },
          { pos.x + 1.f, pos.y },
          0xffc0c0c0);

        float ptsScale = mPixelsPerVideoFrame / videoRender.getPtsDuration();

        { // lock video during iterate
        shared_lock<shared_mutex> lock (videoRender.getSharedMutex());
        for (auto& frame : videoRender.getFrames()) {
          //{{{  draw video frames
          cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame.second);

          float offset1 = (videoFrame->mPts - playerPts) * ptsScale;
          float offset2 = offset1 + (videoFrame->mPtsDuration * ptsScale) - 1.f;

          // pesSize I white / P purple / B blue - ABGR color
          ImGui::GetWindowDrawList()->AddRectFilled (
            { pos.x + offset1,
              pos.y - addValue ((float)videoFrame->mPesSize, mMaxPesSize, mMaxDisplayPesSize,
                                kLines * ImGui::GetTextLineHeight()) },
            { pos.x + offset2, pos.y },
            (videoFrame->mFrameType == 'I') ?
              0xffffffff : (videoFrame->mFrameType == 'P') ?
                0xffFF40ff : 0xffFF4040);
          }
          //}}}
        for (auto frame : videoRender.getFreeFrames()) {
          //{{{  draw free video frames
          cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);

          float offset1 = (videoFrame->mPts - playerPts) * ptsScale;
          float offset2 = offset1 + (videoFrame->mPtsDuration * ptsScale) - 1.f;

          ImGui::GetWindowDrawList()->AddRectFilled (
            { pos.x + offset1,
              pos.y - addValue ((float)videoFrame->mPesSize, mMaxPesSize, mMaxDisplayPesSize,
                                kLines * ImGui::GetTextLineHeight())},
            { pos.x + offset2, pos.y},
            0xFF808080);
          }
          //}}}
        }

        { // lock audio during iterate
        shared_lock<shared_mutex> lock (audioRender.getSharedMutex());
        for (auto& frame : audioRender.getFrames()) {
          //{{{  draw audio frames
          cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame.second);

          float offset1 = (audioFrame->mPts - playerPts) * ptsScale;
          float offset2 = offset1 + (audioFrame->mPtsDuration * ptsScale) - 1.f;

          ImGui::GetWindowDrawList()->AddRectFilled (
            { pos.x + offset1,
              pos.y - addValue (audioFrame->getSimplePower(), mMaxPower, mMaxDisplayPower,
                                kLines * ImGui::GetTextLineHeight()) },
            { pos.x + offset2, pos.y },
            0x8000ff00);
          }
          //}}}
        for (auto frame : audioRender.getFreeFrames()) {
          //{{{  draw free audio frames
          cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);

          float offset1 = (audioFrame->mPts - playerPts) * ptsScale;
          float offset2 = offset1 + (audioFrame->mPtsDuration * ptsScale) - 1.f;

          ImGui::GetWindowDrawList()->AddRectFilled (
            { pos.x + offset1,
              pos.y - addValue (audioFrame->getSimplePower(), mMaxPower, mMaxDisplayPower,
                                kLines * ImGui::GetTextLineHeight()) },
            { pos.x + offset2, pos.y },
            0x80808080);
          }
          //}}}
        }

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
        cAudioFrame* audioFrame = audioRender.findAudioFrameFromPts (playerPts);
        if (audioFrame) {
          size_t drawChannels = audioFrame->mNumChannels;
          bool audio51 = (audioFrame->mNumChannels == 6);
          array <size_t, 6> channelOrder;
          if (audio51) {
            channelOrder = {4, 0, 2, 1, 5, 3};
            drawChannels--;
            }
          else
            channelOrder = {0, 1, 2, 3, 4, 5};

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

    cFramesView mFramesView;
    cAudioMeterView mAudioMeterView;

    // video
    cQuad* mVideoQuad = nullptr;

    // subtitle
    cMat4x4 mModel;
    cRect mRect = { 0,0,0,0 };

    static const size_t kMaxSubtitleLines = 3;
    array <cQuad*,kMaxSubtitleLines> mSubtitleQuads = { nullptr };
    array <cTexture*,kMaxSubtitleLines> mSubtitleTextures = { nullptr };
    };
  //}}}
  //{{{
  class cMultiView {
  public:
    cMultiView() {}
    ~cMultiView() = default;

    // gets
    size_t getNumViews() const { return mServiceView.size(); }
    uint16_t getSelectedSid() const { return mSelectedSid; }

    //{{{
    bool hover() {
    // run for every item to ensure mHover flag get set

      bool result = false;

      for (auto& view : mServiceView)
        if (view.second.hover())
          result = true;;

      mHover = result;
      return result;
      }
    //}}}
    //{{{
    bool picked (cVec2 pos, uint16_t& sid) {
    // pick first view that matches

      for (auto& view : mServiceView)
        if (view.second.picked (pos)) {
          sid = view.first;
          return true;
          }

      return false;
      }
    //}}}
    //{{{
    void select (uint16_t sid) {
    // toggle select

      mSelected = !mSelected;
      if (mSelected)
        mSelectedSid = sid;
      }
    //}}}

    //{{{
    void draw (cTransportStream& transportStream, cGraphics& graphics) {

      if (!cServiceView::mVideoShader)
       cServiceView::mVideoShader = graphics.createTextureShader (cTexture::eYuv420);

      if (!cServiceView::mSubtitleShader)
        cServiceView::mSubtitleShader = graphics.createTextureShader (cTexture::eRgba);

      // update serviceViewMap from enabled services, take care to reuse cServiceView's
      for (auto& pair : transportStream.getServiceMap()) {
        cTransportStream::cService& service = pair.second;

        // find service sid in serviceViewMap
        auto it = mServiceView.find (pair.first);
        if (it == mServiceView.end()) {
          if (service.getRenderStream (eRenderVideo).isEnabled())
            // enabled and not found, add service to serviceViewMap
            mServiceView.emplace (service.getSid(), cServiceView (service));
          }
        else if (!service.getRenderStream (eRenderVideo).isEnabled())
          // found, but not enabled, remove service from serviceViewMap
          mServiceView.erase (it);
        }

      // draw views
      size_t viewIndex = 0;
      for (auto& view : mServiceView) {
        if (!mSelected || (view.first == mSelectedSid))
          view.second.draw (graphics, mSelected, mSelected ? 1 : getNumViews(), viewIndex++);
        else
          view.second.trim();
        }
      }
    //}}}

  private:
    map <uint16_t, cServiceView> mServiceView;

    bool mHover = false;
    bool mSelected = false;
    uint16_t mSelectedSid = 0;
    };
  //}}}

  //{{{
  class cTellyApp : public cApp {
  public:
    //{{{
    cTellyApp (const cPoint& windowSize, bool fullScreen)
      : cApp("telly", windowSize, fullScreen, true) {}
    //}}}
    virtual ~cTellyApp() = default;

    cMultiView& getMultiView() { return mMultiView; }

    bool hasTransportStream() { return mTransportStream; }
    cTransportStream& getTransportStream() { return *mTransportStream; }

    // fileSource
    bool isFileSource() const { return !mFileName.empty(); }
    std::string getFileName() const { return mFileName; }
    uint64_t getFilePos() const { return mFilePos; }
    size_t getFileSize() const { return mFileSize; }

    // dvbSource
    bool isDvbSource() const { return mDvbSource; }
    cDvbSource& getDvbSource() { return *mDvbSource; }

    //{{{
    void fileSource (const string& filename, bool showAllServices) {
    // create fileSource, any channel

      mTransportStream = new cTransportStream (kMultiplexes[0], "", false, showAllServices, true);
      if (mTransportStream) {
        // launch fileSource thread
        mFileName = cFileUtils::resolve (filename);
        FILE* file = fopen (mFileName.c_str(), "rb");
        if (file) {
          thread ([=]() {
            cLog::setThreadName (mFileName);

            size_t blockSize = 188 * 256;
            uint8_t* buffer = new uint8_t[blockSize];

            mFilePos = 0;
            while (true) {
              size_t bytesRead = fread (buffer, 1, blockSize, file);
              if (bytesRead > 0)
                mFilePos += mTransportStream->demux (buffer, bytesRead, mFilePos, false);
              else
                break;

              #ifdef _WIN32
                struct _stati64 st;
                if (_stat64 (mFileName.c_str(), &st) != -1)
                  mFileSize = st.st_size;
              #else
                struct stat st;
                if (stat (mFileName.c_str(), &st) != -1)
                  mFileSize = st.st_size;
              #endif
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
    //{{{
    void liveDvbSource (const cDvbMultiplex& dvbMultiplex, const string& recordRoot, bool showAllServices) {
    // create liveDvbSource from dvbMultiplex

      if (dvbMultiplex.mFrequency)
        mDvbSource = new cDvbSource (dvbMultiplex.mFrequency, 0);

      mTransportStream = new cTransportStream (dvbMultiplex, recordRoot, true, showAllServices, false);
      if (mTransportStream) {
        thread([=, &dvbMultiplex, &recordRoot]() {
          cLog::setThreadName ("dvb");

          FILE* mFile = dvbMultiplex.mRecordAll ?
            fopen ((recordRoot + dvbMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

          #ifdef _WIN32
            //{{{  windows
            mDvbSource->run();

            int64_t streamPos = 0;
            int blockSize = 0;

            while (true) {
              auto ptr = mDvbSource->getBlockBDA (blockSize);
              if (blockSize) {
                //  read and demux block
                if (mFile)
                  fwrite (ptr, 1, blockSize, mFile);

                streamPos += mTransportStream->demux (ptr, blockSize, streamPos, false);
                mDvbSource->releaseBlock (blockSize);
                }
              else
                this_thread::sleep_for (1ms);
              }
            //}}}
          #else
            //{{{  linux
            constexpr int kDvrReadBufferSize = 188 * 256;
            uint8_t* buffer = new uint8_t[kDvrReadBufferSize];

            uint64_t streamPos = 0;
            while (true) {
              int bytesRead = mDvbSource->getBlock (buffer, kDvrReadBufferSize);
              if (bytesRead == 0)
                cLog::log (LOGINFO, fmt::format ("cDvb grabThread no bytes read"));
              else {
                // demux
                streamPos += mTransportStream->demux (buffer, bytesRead, 0, false);
                if (mFile)
                  fwrite (buffer, 1, bytesRead, mFile);
                }
              }
            delete [] buffer;
            //}}}
          #endif

          if (mFile)
            fclose (mFile);

          cLog::log (LOGINFO, "exit");
          }).detach();
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
    cMultiView mMultiView;

    cTransportStream* mTransportStream = nullptr;
    cDvbSource* mDvbSource = nullptr;

    // fileSource
    FILE* mFile = nullptr;
    std::string mFileName;
    uint64_t mFilePos = 0;
    size_t mFileSize = 0;
    };
  //}}}
  //{{{
  class cTellyView {
  public:
    //{{{
    void draw (cApp& app) {

      cTellyApp& tellyApp = (cTellyApp&)app;
      cGraphics& graphics = tellyApp.getGraphics();
      cMultiView& multiView = tellyApp.getMultiView();

      graphics.clear (cPoint((int)ImGui::GetWindowWidth(), (int)ImGui::GetWindowHeight()));

      // draw multiView tellys as background
      if (tellyApp.hasTransportStream())
        multiView.draw (tellyApp.getTransportStream(), graphics);

      //{{{  draw tabs
      ImGui::SameLine();

      mTab = (eTab)interlockedButtons (kTabNames, (uint8_t)mTab, {0.f,0.f}, true);
      //}}}
      //{{{  draw fullScreen
      if (tellyApp.getPlatform().hasFullScreen()) {
        ImGui::SameLine();
        if (toggleButton ("full", tellyApp.getPlatform().getFullScreen()))
          tellyApp.getPlatform().toggleFullScreen();
        }
      //}}}
      //{{{  draw vsync
      ImGui::SameLine();

      if (tellyApp.getPlatform().hasVsync())
        if (toggleButton ("vsync", tellyApp.getPlatform().getVsync()))
          tellyApp.getPlatform().toggleVsync();
      //}}}
      //{{{  draw frameRate
      ImGui::SameLine();

      ImGui::TextUnformatted (fmt::format ("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      //}}}

      if (tellyApp.hasTransportStream()) {
        cTransportStream& transportStream = tellyApp.getTransportStream();
        if (transportStream.hasTdtTime()) {
          //{{{  draw tdtTime
          ImGui::SameLine();
          ImGui::TextUnformatted (transportStream.getTdtTimeString().c_str());
          }
          //}}}
        if (tellyApp.isFileSource()) {
          //{{{  draw filePos
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{:4.3f}%", tellyApp.getFilePos() * 100.f /
                                                            tellyApp.getFileSize()).c_str());
          }
          //}}}
        else if (tellyApp.isDvbSource()) {
          //{{{  draw cTransportStream::dvbSource signal:errors
          ImGui::SameLine();
          ImGui::TextUnformatted (fmt::format ("{}:{}", tellyApp.getDvbSource().getTuneString(),
                                                        tellyApp.getDvbSource().getStatusString()).c_str());
          }
          //}}}
        //{{{  draw transportStream packet,errors
        ImGui::SameLine();
        ImGui::TextUnformatted (fmt::format ("{}:{}", tellyApp.getTransportStream().getNumPackets(),
                                                      tellyApp.getTransportStream().getNumErrors()).c_str());
        //}}}

        // draw tab childWindow, monospaced font
        ImGui::PushFont (tellyApp.getMonoFont());
        ImGui::BeginChild ("tab", {0.f,0.f}, false,
                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_HorizontalScrollbar);
        switch (mTab) {
          case eTellyChan: drawChannels (transportStream, graphics); break;
          case eServices:  drawServices (transportStream, graphics); break;
          case ePids:      drawPidMap (transportStream, graphics); break;
          case eRecorded:  drawRecorded (transportStream, graphics); break;
          default:;
          }

        multiView.hover();
        if (ImGui::IsMouseClicked (0)) {
          uint16_t sid = 0;
          if (multiView.picked (cVec2 (ImGui::GetMousePos().x, ImGui::GetMousePos().y), sid))
            multiView.select (sid);
          else
            tellyApp.getPlatform().toggleFullScreen();
          }

        keyboard();

        ImGui::EndChild();
        ImGui::PopFont();
        }
      }
    //}}}

  private:
    enum eTab { eTelly, eTellyChan, eServices, ePids, eRecorded };
    inline static const vector<string> kTabNames = { "telly", "channels", "services", "pids", "recorded" };
    //{{{
    void drawChannels (cTransportStream& transportStream, cGraphics& graphics) {
      (void)graphics;

      // draw services/channels
      for (auto& pair : transportStream.getServiceMap()) {
        cTransportStream::cService& service = pair.second;
        if (ImGui::Button (fmt::format ("{:{}s}", service.getChannelName(), mMaxNameChars).c_str()))
          service.toggleAll();

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

      for (auto& pair : transportStream.getServiceMap()) {
        // iterate services
        cTransportStream::cService& service = pair.second;
        //{{{  update button maxChars
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
                           service.getProgramPid(), mMaxPgmChars,
                           service.getSid(), mMaxSidChars).c_str()))
          service.toggleAll();

        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
         // iterate definedStreams
          cTransportStream::cStream& stream = service.getRenderStream (eRenderType(renderType));
          if (stream.isDefined()) {
            ImGui::SameLine();
            // draw definedStream button - sid ensuresd unique button name
            if (toggleButton (fmt::format ("{}{:{}d}:{}##{}",
                                           stream.getLabel(),
                                           stream.getPid(), mPidMaxChars[renderType], stream.getTypeName(),
                                           service.getSid()).c_str(), stream.isEnabled()))
             transportStream.toggleStream (service, eRenderType(renderType));
            }
          }

        if (service.getChannelRecord()) {
          //{{{  draw record pathName
          ImGui::SameLine();
          ImGui::Button (fmt::format ("rec:{}##{}",
                                      service.getChannelRecordName(), service.getSid()).c_str());
          }
          //}}}

        // audio provides playPts
        int64_t playPts = 0;
        if (service.getRenderStream (eRenderAudio).isEnabled())
          playPts = dynamic_cast<cAudioRender&>(service.getRenderStream (eRenderAudio).getRender()).getPlayer().getPts();

        for (uint8_t renderType = eRenderVideo; renderType <= eRenderSubtitle; renderType++) {
          // iterate enabledStreams, drawing
          cTransportStream::cStream& stream = service.getRenderStream (eRenderType(renderType));
          if (stream.isEnabled()) {
            switch (eRenderType(renderType)) {
              case eRenderVideo:
                drawVideoInfo (service.getSid(), stream.getRender(), graphics, playPts); break;

              case eRenderAudio:
              case eRenderDescription:
                drawAudioInfo (service.getSid(), stream.getRender(), graphics);  break;

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
    void drawPidMap (cTransportStream& transportStream, cGraphics& graphics) {
    // draw pids
      (void)graphics;

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
    void drawRecorded (cTransportStream& transportStream, cGraphics& graphics) {
      (void)graphics;

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
    void drawAudioInfo (uint16_t sid, cRender& render, cGraphics& graphics) {
      (void)sid;
      (void)graphics;

      cAudioRender& audioRender = dynamic_cast<cAudioRender&>(render);

      ImGui::TextUnformatted (audioRender.getInfoString().c_str());

      if (ImGui::InvisibleButton (fmt::format ("##audLog{}", sid).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        audioRender.toggleLog();

      drawMiniLog (audioRender.getLog());
      }
    //}}}
    //{{{
    void drawVideoInfo (uint16_t sid, cRender& render, cGraphics& graphics, int64_t playPts) {
      (void)sid;
      (void)graphics;
      (void)playPts;

      cVideoRender& videoRender = dynamic_cast<cVideoRender&>(render);
      ImGui::TextUnformatted (videoRender.getInfoString().c_str());

      if (ImGui::InvisibleButton (fmt::format ("##vidLog{}", sid).c_str(),
                                  {4 * ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight()}))
        videoRender.toggleLog();

      drawMiniLog (videoRender.getLog());
      }
    //}}}

    // not yet
    //{{{
    void undo() {
      cLog::log (LOGINFO, "undo");
      }
    //}}}
    //{{{
    void redo() {
      cLog::log (LOGINFO, "redo");
      }
    //}}}
    //{{{
    void toggleFullScreen() {
      cLog::log (LOGINFO, "toggleFullScreen");
      }
    //}}}

    //{{{
    void keyboard() {

      ImGui::GetIO().WantTextInput = true;
      ImGui::GetIO().WantCaptureKeyboard = false;

      bool altKeyPressed = ImGui::GetIO().KeyAlt;
      bool ctrlKeyPressed = ImGui::GetIO().KeyCtrl;
      bool shiftKeyPressed = ImGui::GetIO().KeyShift;

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

    array <size_t, 4> mPidMaxChars = { 3 };
    //array <size_t, 4> mPidMaxChars = { 3 };

    float mOverlap = 4.f;
    int mHistory = 0;

    array <cTexture*,4> mSubtitleTextures = { nullptr };
    };
  //}}}
  //{{{
  // cTellyUI
  class cTellyUI : public cUI {
  public:
    cTellyUI (const string& name) : cUI(name) {}
    virtual ~cTellyUI() = default;

    //{{{
    void addToDrawList (cApp& app) final {
    // draw into window

      ImGui::SetNextWindowPos (ImVec2(0,0));
      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);

      ImGui::Begin ("telly", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);
      mTellyView.draw (app);
      ImGui::End();
      }
    //}}}

  private:
    // self registration
    static cUI* create (const string& className) { return new cTellyUI (className); }

    // static var self registration trick
    inline static const bool mRegistered = registerClass ("telly", &create);

    // vars
    cTellyView mTellyView;
    };
  //}}}
  }

// main
int main (int numArgs, char* args[]) {

  // params
  bool recordAll = false;
  bool showAllServices = true;
  bool fullScreen = false;
  eLogLevel logLevel = LOGINFO;
  cDvbMultiplex selectedMultiplex = kMultiplexes[1];
  string filename;
  //{{{  parse commandLine to params
  // parse params
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];

    if (param == "all")
      recordAll = true;
    else if (param == "simple")
      showAllServices = false;
    else if (param == "full")
      fullScreen = true;
    else if (param == "log1")
      logLevel = LOGINFO1;
    else if (param == "log2")
      logLevel = LOGINFO2;
    else if (param == "log3")
      logLevel = LOGINFO3;
    else {
      // assume filename
      filename = param;

      // look for named multiplex
      for (auto& multiplex : kMultiplexes) {
        if (param == multiplex.mName) {
          // found named multiplex
          selectedMultiplex = multiplex;
          filename = "";
          break;
          }
        }
      }
    }

  selectedMultiplex.mRecordAll = recordAll;
  //}}}

  // log
  cLog::init (logLevel);
  cLog::log (LOGNOTICE, "tellyApp - all simple full log1 log2 log3 multiplexName filename");

  // list static registered UI classes
  cUI::listRegisteredClasses();

  // app
  cTellyApp tellyApp ({1920/2, 1080/2}, fullScreen);
  tellyApp.setMainFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&itcSymbolBold, itcSymbolBoldSize, 18.f));
  tellyApp.setMonoFont (ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF (&droidSansMono, droidSansMonoSize, 18.f));
  if (filename.empty()) {
    cLog::log (LOGINFO, fmt::format ("- using multiplex {}", selectedMultiplex.mName));
    tellyApp.liveDvbSource (selectedMultiplex, kRootDir, showAllServices);
    }
  else
    tellyApp.fileSource (filename, showAllServices);
  tellyApp.mainUILoop();

  return EXIT_SUCCESS;
  }

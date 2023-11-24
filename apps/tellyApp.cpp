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

        playPts = audioRender.getPlayer().getPts();
        }
        //}}}

      if (!selectFull || getSelected()) {
        // view selected or no views selected
        float viewportWidth = ImGui::GetWindowWidth();
        float viewportHeight = ImGui::GetWindowHeight();
        cMat4x4 projection (0.f,viewportWidth, 0.f,viewportHeight, -1.f,1.f);

        float scale = getScale (numViews);

        // show nearest videoFrame to playPts
        cVideoFrame* videoFrame = videoRender.getVideoNearestFrameFromPts (playPts);
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

          audioRender.getPlayer().setMute (getUnselected());

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

      videoRender.trimVideoBeforePts (playPts);
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
    //{{{
    cTellyApp (const cPoint& windowSize, bool fullScreen)
      : cApp("telly", windowSize, fullScreen, true) {}
    //}}}
    virtual ~cTellyApp() = default;

    cMultiView& getMultiView() { return mMultiView; }

    bool hasTransportStream() { return mTransportStream; }
    cTransportStream& getTransportStream() { return *mTransportStream; }

    bool showSubtitle() const { return mShowSubtitle; }
    void toggleShowSubtitle() { mShowSubtitle = !mShowSubtitle; }

    // fileSource
    bool isFileSource() const { return !mFileName.empty(); }
    std::string getFileName() const { return mFileName; }
    uint64_t getFilePos() const { return mFilePos; }
    size_t getFileSize() const { return mFileSize; }
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

    // dvbSource
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

      mTransportStream = new cTransportStream (multiplex, recordRoot, true, showAllServices, false);
      if (mTransportStream) {
        thread([=]() {
          cLog::setThreadName ("dvb");

          FILE* mFile = mMultiplex.mRecordAll ?
            fopen ((mRecordRoot + mMultiplex.mName + ".ts").c_str(), "wb") : nullptr;

          #ifdef _WIN32
            //{{{  windows
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
    cTransportStream* mTransportStream = nullptr;

    cMultiView mMultiView;
    bool mShowSubtitle = true;

    // fileSource
    FILE* mFile = nullptr;
    std::string mFileName;
    uint64_t mFilePos = 0;
    size_t mFileSize = 0;

    cDvbSource* mDvbSource = nullptr;
    cDvbMultiplex mMultiplex;
    string mRecordRoot;
    };
  //}}}
  //{{{
  class cTellyView {
  public:
    //{{{
    void draw (cApp& app) {

      cTellyApp& tellyApp = (cTellyApp&)app;
      app.getGraphics().clear ({ (int32_t)ImGui::GetIO().DisplaySize.x,
                                 (int32_t)ImGui::GetIO().DisplaySize.y });

      ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
      ImGui::Begin ("telly", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
                                      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar);

      // draw multiView
      if (tellyApp.hasTransportStream())
        tellyApp.getMultiView().draw (tellyApp.getTransportStream(), app.getGraphics(), tellyApp.showSubtitle());

      ImGui::SetCursorPos ({ 0.f,0.f });
      mTab = (eTab)interlockedButtons (kTabNames, (uint8_t)mTab, {0.f,0.f}, true);
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

        // draw tabs usinfg monoSpaced font
        ImGui::PushFont (app.getMonoFont());
        switch (mTab) {
          case eTellyChan: drawChannels (transportStream); break;
          case eServices:  drawServices (transportStream, app.getGraphics()); break;
          case ePidMap:    drawPidMap (transportStream); break;
          case eRecorded:  drawRecorded (transportStream); break;
          default:;
          }
        ImGui::PopFont();

        // invisible bgnd button for mouse
        ImGui::SetCursorPos ({ 0.f,0.f });
        if (ImGui::InvisibleButton ("bgnd", { ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y }))
          tellyApp.getMultiView().selectById (
            tellyApp.getMultiView().pick ({ ImGui::GetMousePos().x, ImGui::GetMousePos().y }));
        tellyApp.getMultiView().hover();

        keyboard();

        ImGui::End();
        }
      }
    //}}}

  private:
    enum eTab { eTelly, eTellyChan, eServices, ePidMap, eRecorded };
    inline static const vector<string> kTabNames = { "telly", "channels", "services", "pids", "recorded" };

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
    void drawRecorded (cTransportStream& transportStream) {

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

    // !!! wrong !!! needed per service
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

      mTellyView.draw (app);
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
  if (filename.empty())
    tellyApp.liveDvbSource (selectedMultiplex, kRootDir, showAllServices);
  else
    tellyApp.fileSource (filename, showAllServices);
  tellyApp.mainUILoop();

  return EXIT_SUCCESS;
  }

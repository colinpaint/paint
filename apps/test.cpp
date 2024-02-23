// test.cpp
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

// dvb
#include "../dvb/cTransportStream.h"

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
#include "../decoders/cVideoFrame.h"
#include "../decoders/cFFmpegVideoFrame.h"
#include "../decoders/cDecoder.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/myImgui.h"

using namespace std;
using namespace utils;
//}}}
constexpr bool kMotionVectors = true;
namespace {
  //{{{
  void logCallback (void* ptr, int level, const char* fmt, va_list vargs) {
    (void)level;
    (void)ptr;
    (void)fmt;
    (void)vargs;
    //vprintf (fmt, vargs);
    }
  //}}}
  }
//{{{
class cVideoDecoder : public cDecoder {
public:
  //{{{
  cVideoDecoder (bool h264) :
     cDecoder(), mH264(h264), mStreamName(h264 ? "h264" : "mpeg2"),
     mAvCodec(avcodec_find_decoder (h264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO)) {

    av_log_set_level (AV_LOG_ERROR);
    av_log_set_callback (logCallback);

    cLog::log (LOGINFO, fmt::format ("cVideoDecoder ffmpeg - {}", mStreamName));

    mAvParser = av_parser_init (mH264 ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    mAvContext->flags2 |= AV_CODEC_FLAG2_FAST;

    if (kMotionVectors) {
      AVDictionary* opts = nullptr;
      av_dict_set (&opts, "flags2", "+export_mvs", 0);
      avcodec_open2 (mAvContext, mAvCodec, &opts);
      av_dict_free (&opts);
      }
    else
      avcodec_open2 (mAvContext, mAvCodec, nullptr);
    }
  //}}}
  //{{{
  virtual ~cVideoDecoder() {

    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  virtual string getInfoString() const final { return mH264 ? "ffmpeg h264" : "ffmpeg mpeg"; }
  virtual void flush() final { avcodec_flush_buffers (mAvContext); }
  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, char frameType,
                          function<cFrame*(int64_t pts)> allocFrameCallback,
                          function<void (cFrame* frame)> addFrameCallback) final {
    AVFrame* avFrame = av_frame_alloc();
    AVPacket* avPacket = av_packet_alloc();

    uint8_t* frame = pes;
    uint32_t frameSize = pesSize;
    while (frameSize) {
      auto now = chrono::system_clock::now();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                        frame, (int)frameSize, pts, AV_NOPTS_VALUE, 0);
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;

          if (frameType == 'I')
            mSeqPts = pts;

          int64_t duration = (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num;

          // alloc videoFrame
          cFFmpegVideoFrame* frame = dynamic_cast<cFFmpegVideoFrame*>(allocFrameCallback (mSeqPts));
          if (frame) {
            frame->set (mSeqPts, duration, frameSize);
            frame->mFrameType = frameType;
            frame->setAVFrame (avFrame, kMotionVectors);
            frame->addTime (chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - now).count());

            // addFrame
            addFrameCallback (frame);

            avFrame = av_frame_alloc();
            }
          mSeqPts += duration;
          }
        }
      frame += bytesUsed;
      frameSize -= bytesUsed;
      }

    av_frame_unref (avFrame);
    av_frame_free (&avFrame);

    av_packet_free (&avPacket);
    return mSeqPts;
    }
  //}}}

private:
  const bool mH264 = false;
  const string mStreamName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;

  bool mGotIframe = false;
  int64_t mSeqPts = -1;
  };
//}}}
//{{{
class cFilePlayer {
public:
  cFilePlayer (string fileName) : mFileName(cFileUtils::resolve (fileName)) {}
  //{{{
  virtual ~cFilePlayer() {
    mPes.clear();
    }
  //}}}

  string getFileName() const { return mFileName; }
  cTransportStream::cService* getService() { return mService; }
  int64_t getPlayPts() const { return mPlayPts; }
  cVideoFrame* getVideoFrame() { return mVideoFrame; }

  void togglePlay() { mPlaying = !mPlaying; }
  //{{{
  void skipPlay (int64_t skipPts) {

    int64_t fromPts = mPlayPts;
    mPlayPts += skipPts;

    auto it = mPes.begin();
    while (it != mPes.end()) {
      if (mPlayPts == it->mPts)
        break;
      ++it;
      }

    if (it != mPes.end()) {
      cLog::log (LOGINFO, fmt::format ("skipPlay from:{} to:{} skip:{}",
                                       getPtsString (fromPts), getPtsString (mPlayPts), skipPts));
      decodePes (*it);
      }
    }
  //}}}
  //{{{
  void skipIframe (int64_t inc) {

    auto it = mPes.begin();
    while (it != mPes.end()) {
      if (mPlayPts >= it->mPts)
        break;
      ++it;
      }

    if (it != mPes.end()) {
      if (inc > 0) {
        while (it != mPes.end()) {
          if (it->mFrameType == 'I')
            break;
          else
            ++it;
          }
        }

      else if (inc < 0) {
        while (it != mPes.begin()) {
          --it;
          if (it->mFrameType == 'I')
            break;
          }
        }

      cLog::log (LOGINFO, fmt::format ("skip Iframe {}", inc));
      mPlayPts = it->mPts;
      decodePes (*it);
      }
    }
  //}}}

  //{{{
  void read() {

    FILE* file = fopen (mFileName.c_str(), "rb");
    if (!file) {
      //{{{  error, return
      cLog::log (LOGERROR, fmt::format ("cFilePlayer::analyse to open {}", mFileName));
      return;
      }
      //}}}
    //{{{  get fileSize
    #ifdef _WIN32
      struct _stati64 st;
      if (_stat64 (mFileName.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #else
      struct stat st;
      if (stat (mFileName.c_str(), &st) != -1)
        mFileSize = st.st_size;
    #endif
    //}}}

    thread ([=]() {
      cLog::setThreadName ("anal");

      mTransportStream = new cTransportStream (
        {"anal", 0, {}, {}}, nullptr,
        // addService lambda
        [&](cTransportStream::cService& service) noexcept {
          mService = &service;
          createDecoder();
          },

         // pes lambda
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo) noexcept {
          if (pidInfo.getPid() == service.getVideoPid()) {
            char frameType = cDvbUtils::getFrameType (
              pidInfo.mBuffer, pidInfo.getBufSize(), service.getVideoStreamTypeId() == 27);
            uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
            memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());
            cPes pes(buffer, pidInfo.getBufSize(), pidInfo.getPts(), frameType);
            mPes.push_back (pes);
            }
          });

      // file read
      size_t chunkSize = 188 * 256;
      uint8_t* chunk = new uint8_t[chunkSize];
      auto now = chrono::system_clock::now();
      while (true) {
        size_t bytesRead = fread (chunk, 1, chunkSize, file);
        if (bytesRead > 0)
          mTransportStream->demux (chunk, bytesRead);
        else
          break;
        }
      //{{{  report totals
      cLog::log (LOGINFO, fmt::format ("{:4.1f}m took {:3.2f}s",
        mFileSize/1000000.f,
        chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count() / 1000.f));

      cLog::log (LOGINFO, fmt::format ("- vid:{:6d} {} to {}",
                                       mPes.size(),
                                       getFullPtsString (mPes.front().mPts),
                                       getFullPtsString (mPes.back().mPts)
                                       ));
                                       //}}}
      cLog::log (LOGINFO, "---------------------------------------------------");

      mDecoder->flush();
      for (auto it = mPes.begin(); it != mPes.end(); ++it) {
        if (it->mFrameType == 'I')
          decodePes (*it);

        while (!mPlaying)
          this_thread::sleep_for (10ms);
        }

      delete[] chunk;
      fclose (file);

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
  //}}}

private:
  static inline const size_t kVideoFrames = 8;
  //{{{
  class cPes {
  public:
    cPes (uint8_t* data, uint32_t size, int64_t pts, char frameType) :
      mData(data), mSize(size), mPts(pts), mFrameType(frameType) {}

    uint8_t* mData;
    uint32_t mSize;
    int64_t mPts;
    char mFrameType;
    };
  //}}}

  //{{{
  void createDecoder() {

    mDecoder = new cVideoDecoder (true);

    for (int i = 0; i < kVideoFrames;  i++)
      mVideoFrames[i] = new cFFmpegVideoFrame();
    }
  //}}}
  //{{{
  void decodePes (cPes pes) {

    if (mDecoder) {
      cLog::log (LOGINFO, fmt::format ("decode {} {}", pes.mFrameType, getPtsString (pes.mPts)));

      mDecoder->decode (pes.mData, pes.mSize, pes.mPts, pes.mFrameType,
        // allocFrame lambda
        [&](int64_t pts) noexcept {
          mVideoFrameIndex = (mVideoFrameIndex + 1) % kVideoFrames;
          mVideoFrames[mVideoFrameIndex]->releaseResources();
          return mVideoFrames[mVideoFrameIndex];
          },

        // addFrame lambda
        [&](cFrame* frame) noexcept {
          cLog::log (LOGINFO, fmt::format ("- addFrame {}", getPtsString (frame->getPts())));
          cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
          videoFrame->mTextureDirty = true;
          mVideoFrame = videoFrame;
          });
      }
    }
  //}}}

  string mFileName;
  size_t mFileSize = 0;

  cTransportStream* mTransportStream = nullptr;
  cTransportStream::cService* mService = nullptr;

  bool mGotIframe = false;
  vector <cPes> mPes;

  // playing
  int64_t mPlayPts = -1;
  bool mPlaying = true;

  // videoDecoder
  int mVideoFrameIndex = -1;
  array <cVideoFrame*,8> mVideoFrames = { nullptr };
  cVideoFrame* mVideoFrame = nullptr;
  cDecoder* mDecoder = nullptr;
  };
//}}}
//{{{
class cTestApp : public cApp {
public:
  cTestApp (cApp::cOptions* options, iUI* ui) : cApp ("Test", options, ui), mOptions(options) {}
  virtual ~cTestApp() {}

  void addFile (const string& fileName) {
    mFilePlayer = new cFilePlayer (fileName);
    mFilePlayer->read();
    }

  cApp::cOptions* getOptions() { return mOptions; }
  cFilePlayer* getFilePlayer() { return mFilePlayer; }

  void drop (const vector<string>& dropItems) {
    for (auto& item : dropItems) {
      cLog::log (LOGINFO, fmt::format ("cPlayerApp::drop {}", item));
      addFile (item);
      }
    }

private:
  cApp::cOptions* mOptions;
  cFilePlayer* mFilePlayer = nullptr;
  };
//}}}
//{{{
class cView {
public:
  cView (cTransportStream::cService* service) : mService(service) {}
  ~cView() = default;

  void draw (cTestApp& testApp, cTextureShader* videoShader) {
    float layoutScale;
    cVec2 layoutPos = getLayout (0, 1, layoutScale);
    float viewportWidth = ImGui::GetWindowWidth();
    float viewportHeight = ImGui::GetWindowHeight();
    mSize = {layoutScale * viewportWidth, layoutScale * viewportHeight};
    mTL = {(layoutPos.x * viewportWidth) - mSize.x*0.5f, (layoutPos.y * viewportHeight) - mSize.y*0.5f};
    mBR = {(layoutPos.x * viewportWidth) + mSize.x*0.5f, (layoutPos.y * viewportHeight) + mSize.y*0.5f};

    ImGui::SetCursorPos (mTL);
    ImGui::BeginChild (fmt::format ("view##{}", mService->getSid()).c_str(), mSize,
                       ImGuiChildFlags_None,
                       ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                       ImGuiWindowFlags_NoBackground);

    if (testApp.getFilePlayer()) {
      cVideoFrame* videoFrame = testApp.getFilePlayer()->getVideoFrame();
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
        cTexture& texture = videoFrame->getTexture (testApp.getGraphics());
        texture.setSource();

        // ensure quad is created
        if (!mVideoQuad)
          mVideoQuad = testApp.getGraphics().createQuad (videoFrame->getSize());

        // draw quad
        mVideoQuad->draw();
        //}}}
        //{{{  draw frame info
        string title = fmt::format ("{} {} {} {}",
                                    videoFrame->getFrameType(),
                                    getPtsString(videoFrame->getPts()),
                                    videoFrame->getPtsDuration(),
                                    videoFrame->getPesSize()
                                    );

        ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
        pos = { ImGui::GetTextLineHeight(), mSize.y - 3 * ImGui::GetTextLineHeight()};
        ImGui::SetCursorPos (pos);
        ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
        ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
        ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
        //}}}
        }
      }
    //{{{  title, playPts
    ImGui::PushFont (testApp.getLargeFont());

    string title = testApp.getFilePlayer()->getFileName();
    ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
    ImGui::SetCursorPos (pos);
    ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
    ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
    ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());

    string ptsString = getPtsString (testApp.getFilePlayer()->getPlayPts());
    pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
    ImGui::SetCursorPos (pos);
    ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsString.c_str());
    ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
    ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsString.c_str());

    ImGui::PopFont();
    //}}}
    ImGui::EndChild();
    }

private:
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
  cTransportStream::cService* mService;

  // video
  ImVec2 mSize;
  ImVec2 mTL;
  ImVec2 mBR;
  cQuad* mVideoQuad = nullptr;
  };
//}}}
//{{{
class cTestUI : public cApp::iUI {
public:
  virtual ~cTestUI() = default;

  void draw (cApp& app) {
    ImGui::SetKeyboardFocusHere();
    app.getGraphics().clear ({(int32_t)ImGui::GetIO().DisplaySize.x,
                              (int32_t)ImGui::GetIO().DisplaySize.y});

    // draw UI
    ImGui::SetNextWindowPos ({0.f,0.f});
    ImGui::SetNextWindowSize (ImGui::GetIO().DisplaySize);
    ImGui::Begin ("test", nullptr, ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoBackground);

    cTestApp& testApp = (cTestApp&)app;
    if (!mVideoShader)
      mVideoShader = testApp.getGraphics().createTextureShader (cTexture::eYuv420);

    cFilePlayer* filePlayer = testApp.getFilePlayer();
    if (filePlayer && filePlayer->getService()) {
      if (!mView)
        mView = new cView (filePlayer->getService());
      mView->draw (testApp, mVideoShader);

      // draw menu
      ImGui::SetCursorPos ({0.f, ImGui::GetIO().DisplaySize.y - ImGui::GetTextLineHeight() * 1.5f});
      ImGui::BeginChild ("menu", {0.f, ImGui::GetTextLineHeight() * 1.5f},
                         ImGuiChildFlags_None,
                         ImGuiWindowFlags_NoBackground);

      ImGui::SetCursorPos ({0.f,0.f});
      //{{{  draw fullScreen button
      ImGui::SameLine();
      if (toggleButton ("full", testApp.getPlatform().getFullScreen()))
        testApp.getPlatform().toggleFullScreen();
      //}}}
      //{{{  draw frameRate info
      ImGui::SameLine();
      ImGui::TextUnformatted (fmt::format("{}:fps", static_cast<uint32_t>(ImGui::GetIO().Framerate)).c_str());
      //}}}
      ImGui::EndChild();
      }
    ImGui::End();
    keyboard (testApp);
    }

private:
  //{{{
  void keyboard (cTestApp& testApp) {

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
      { false, false,  false, ImGuiKey_Space,      [this,&testApp]{ testApp.getFilePlayer()->togglePlay(); }},

      { false, false,  false, ImGuiKey_LeftArrow,  [this,&testApp]{ testApp.getFilePlayer()->skipPlay (-90000/25); }},
      { false, false,  false, ImGuiKey_RightArrow, [this,&testApp]{ testApp.getFilePlayer()->skipPlay (90000/25); }},
      { false, true,   false, ImGuiKey_LeftArrow,  [this,&testApp]{ testApp.getFilePlayer()->skipPlay (-90000); }},
      { false, true,   false, ImGuiKey_RightArrow, [this,&testApp]{ testApp.getFilePlayer()->skipPlay (90000); }},

      { false, false,  false, ImGuiKey_UpArrow,    [this,&testApp]{ testApp.getFilePlayer()->skipIframe (-1); }},
      { false, false,  false, ImGuiKey_DownArrow,  [this,&testApp]{ testApp.getFilePlayer()->skipIframe (1); }},
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
  cView* mView;
  cTextureShader* mVideoShader = nullptr;
  };
//}}}

// main
int main (int numArgs, char* args[]) {

  // options
  string fileName;
  cApp::cOptions* options = new cApp::cOptions();
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) // found cApp option
      ;
    else
      fileName = param;
    }

  // log
  cLog::init (options->mLogLevel);

  // launch
  cTestApp testApp (options, new cTestUI());
  testApp.addFile (fileName);
  testApp.mainUILoop();

  return EXIT_SUCCESS;
  }

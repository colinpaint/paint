// cVideoRender.cpp
//{{{  includes
#include "cVideoRender.h"
#include "cVideoFrame.h"

#ifdef _WIN32
  //{{{  windows headers
  #define NOMINMAX

  #include <intrin.h>

  #include <initguid.h>
  #include <d3d9.h>
  #include <dxva2api.h>

  #include <d3d11.h>
  #include <dxgi1_2.h>

  #define D3DFMT_NV12 (D3DFORMAT)MAKEFOURCC('N','V','1','2')
  #define D3DFMT_YV12 (D3DFORMAT)MAKEFOURCC('Y','V','1','2')

  #define WILL_READ 0x1000
  #define WILL_WRITE 0x2000
  //}}}
#endif

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <algorithm>
#include <thread>
#include <functional>

#include "../dvb/cDvbUtils.h"
#include "../app/cGraphics.h"

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/utils.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

using namespace std;
//}}}
constexpr bool kQueued = true;
constexpr uint32_t kVideoFrameMapSize = 30;

// cFrame derived classes
//{{{
class cFFmpegVideoFrame : public cVideoFrame {
public:
  cFFmpegVideoFrame() : cVideoFrame(cTexture::eYuv420) {}
  //{{{
  virtual ~cFFmpegVideoFrame() {
    releasePixels();
    }
  //}}}

  void setAVFrame (AVFrame* avFrame) { mAvFrame = avFrame; }

protected:
  virtual uint8_t** getPixels() final { return mAvFrame->data; }
  virtual int* getStrides() final { return mAvFrame->linesize; }

  //{{{
  virtual void releasePixels() final {

    if (mAvFrame) {
      av_frame_unref (mAvFrame);
      av_frame_free (&mAvFrame);
      mAvFrame = nullptr;
      }
    }
  //}}}

private:
  AVFrame* mAvFrame = nullptr;
  };
//}}}

// cDecoder derived classes
//{{{
class cFFmpegDecoder : public cDecoder {
public:
  //{{{
  cFFmpegDecoder (uint8_t streamTypeId)
     : cDecoder(), mH264 (streamTypeId == 27),
       mAvCodec (avcodec_find_decoder ((streamTypeId == 27) ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO)) {

    cLog::log (LOGINFO, fmt::format ("cFFmpegDecoder with streamTypeId:{}",
                                     streamTypeId, streamTypeId == 27 ? "h264" : "mpeg2"));

    mAvParser = av_parser_init ((streamTypeId == 27) ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    avcodec_open2 (mAvContext, mAvCodec, NULL);
    }
  //}}}
  //{{{
  virtual ~cFFmpegDecoder() {

    if (mAvContext)
      avcodec_close (mAvContext);

    if (mAvParser)
      av_parser_close (mAvParser);

    if (mSwsContext)
      sws_freeContext (mSwsContext);
    }
  //}}}

  virtual string getName() const final { return "ffmpeg"; }

  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          function<cFrame*()> allocFrameCallback,
                          function<void (cFrame* frame)> addFrameCallback) final {

    cLog::log (LOGINFO, fmt::format ("cVideoRender::decode {} pts:{} dts:{} pesSize:{}",
                                     mH264 ? "h264" : "mpeg2",
                                     utils::getPtsString (pts), utils::getPtsString (dts),
                                     pesSize));
    AVFrame* avFrame = av_frame_alloc();
    AVPacket* avPacket = av_packet_alloc();

    uint8_t* frame = pes;
    uint32_t frameSize = pesSize;
    while (frameSize) {
      auto now = chrono::system_clock::now();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                        frame, (int)frameSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;

          cDvbUtils::getFrameType (frame, frameSize, mH264);

          char frameType = '?';
          if (avFrame->pict_type == AV_PICTURE_TYPE_I)
            frameType = 'I';
          else if (avFrame->pict_type == AV_PICTURE_TYPE_P)
            frameType = 'P';
          else if (avFrame->pict_type == AV_PICTURE_TYPE_B)
             frameType = 'B';

          if (frameType == 'I') {
            //{{{  pts seems wrong, frames decode in presentation order, only correct on Iframe
            if ((mInterpolatedPts != -1) && (mInterpolatedPts != dts))
              cLog::log (LOGERROR, fmt::format ("- lost:{} pts:{} dts:{} size:{}",
                                                frameType, utils::getPtsString (pts), utils::getPtsString (dts), pesSize));
            mInterpolatedPts = dts;
            mGotIframe = true;
            }
            //}}}
          if (!mGotIframe) {
            //{{{  skip until first Iframe
            cLog::log (LOGINFO, fmt::format ("skip:{} pts:{} dts:{} size:{}",
                                             frameType, utils::getPtsString (pts), utils::getPtsString (dts), pesSize));
            return pts;
            }
            //}}}

          // allocFrame
          cFFmpegVideoFrame* videoFrame = dynamic_cast<cFFmpegVideoFrame*>(allocFrameCallback());
          videoFrame->mFrameType = frameType;
          videoFrame->mPts = mInterpolatedPts;
          videoFrame->mPtsDuration = (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num;
          videoFrame->mPesSize = frameSize;
          videoFrame->mWidth = static_cast<uint16_t>(avFrame->width);
          videoFrame->mHeight = static_cast<uint16_t>(avFrame->height);
          videoFrame->mStrideY = static_cast<uint16_t>(avFrame->height);
          videoFrame->mStrideUV = static_cast<uint16_t>(avFrame->height);
          videoFrame->addTime (chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - now).count());

          // transfer avFrame to videoFrame, addFrame, alloc new avFrame
          videoFrame->setAVFrame (avFrame);
          addFrameCallback (videoFrame);
          avFrame = av_frame_alloc();

          mInterpolatedPts += videoFrame->mPtsDuration;
          }
        }
      frame += bytesUsed;
      frameSize -= bytesUsed;
      }

    av_frame_unref (avFrame);
    av_frame_free (&avFrame);

    av_packet_free (&avPacket);
    return mInterpolatedPts;
    }
  //}}}

private:
  const bool mH264 = false;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;
  SwsContext* mSwsContext = nullptr;

  bool mGotIframe = false;
  int64_t mInterpolatedPts = -1;
  };
//}}}

// cVideoRender
//{{{
cVideoRender::cVideoRender (const string& name, uint8_t streamTypeId, uint16_t decoderMask)
    : cRender(kQueued, name + "vid", streamTypeId, decoderMask, kVideoFrameMapSize) {

  mDecoder = new cFFmpegDecoder (streamTypeId);
  setAllocFrameCallback ([&]() noexcept { return getFFmpegFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });
  }
//}}}
//{{{
cVideoRender::~cVideoRender() {

  unique_lock<shared_mutex> lock (mSharedMutex);

  for (auto& frame : mFrames)
    delete frame.second;
  mFrames.clear();

  for (auto& frame : mFreeFrames)
    delete frame;
  mFreeFrames.clear();
  }
//}}}

// frames, freeframes
//{{{
cVideoFrame* cVideoRender::getVideoFramePts (int64_t pts) {

  // quick unlocked test
  if (mFrames.empty() || !mPtsDuration) {
    cLog::log (LOGERROR, fmt::format ("cVideoRender::getVideoFramePts failed {} dur:{} frames:{}",
                                      utils::getPtsString (pts), mPtsDuration, mFrames.size()));
    return nullptr;
    }

  // locked
  //shared_lock<shared_mutex> lock (mSharedMutex);
  return dynamic_cast<cVideoFrame*>(getFramePts (pts / mPtsDuration));
  }
//}}}
//{{{
void cVideoRender::trimVideoBeforePts (int64_t pts) {
  trimFramesBeforePts (pts / mPtsDuration);
  }
//}}}

// decoder callbacks
//{{{
cFrame* cVideoRender::getFFmpegFrame() {

  cFrame* frame = getFreeFrame();
  if (frame)
    return frame;

  return new cFFmpegVideoFrame();
  }
//}}}
//{{{
void cVideoRender::addFrame (cFrame* frame) {

  cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
  videoFrame->mQueueSize = getQueueSize();
  videoFrame->mTextureDirty = true;

  mWidth = videoFrame->mWidth;
  mHeight = videoFrame->mHeight;
  mStrideY = videoFrame->mStrideY;
  mStrideUV = videoFrame->mStrideUV;

  mPtsDuration = videoFrame->mPtsDuration;
  mFrameInfo = videoFrame->getInfoString();

  cLog::log (LOGINFO1, fmt::format ("cVideoRender::addFrame {} size:{}x{}",
                                    utils::getPtsString (videoFrame->mPts), videoFrame->mWidth, videoFrame->mHeight));
  // locked
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFrames.emplace (videoFrame->mPts / videoFrame->mPtsDuration, videoFrame);
  }
//}}}

// virtual
//{{{
string cVideoRender::getInfoString() const {
  return fmt::format ("video frames:{:3d}:{:3d} {} qSize:{} {}",
                      mFrames.size(), mFreeFrames.size(), mDecoder->getName(), getQueueSize(),  mFrameInfo);
  }
//}}}

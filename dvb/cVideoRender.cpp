// cVideoRender.cpp
//{{{  includes
#include "cVideoRender.h"

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <array>
#include <algorithm>
#include <thread>
#include <functional>

#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

#include "../dvb/cDvbUtils.h"
#include "../graphics/cGraphics.h"

#include "cFrame.h"

//#include "../libmfx/include/mfxvideo++.h"
#include "mfxvideo++.h"

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

#define INTEL_SSE2
#define INTEL_SSSE3 1
#include <emmintrin.h>
#include <tmmintrin.h>

using namespace std;
//}}}

constexpr uint32_t kVideoPoolSize = 100;
//{{{
class cVideoFrame : public cFrame {
public:
  // factory create
  static cVideoFrame* createVideoFrame (int64_t pts, int64_t ptsDuration);

  cVideoFrame (int64_t pts, int64_t ptsDuration) : cFrame (pts, ptsDuration) {}
  //{{{
  virtual ~cVideoFrame() {

    #ifdef _WIN32
      _aligned_free (mPixels);
    #else
      free (mPixels);
    #endif
    }
  //}}}

  virtual void init (uint16_t width, uint16_t height, uint16_t stride, uint32_t pesSize, int64_t decodeTime) = 0;

  // gets
  uint16_t getWidth() const {return mWidth; }
  uint16_t getHeight() const {return mHeight; }
  uint8_t* getPixels() const { return (uint8_t*)mPixels; }

  uint32_t getPesSize() const { return mPesSize; }
  int64_t getDecodeTime() const { return mDecodeTime; }
  int64_t getYuvRgbTime() const { return mYuvRgbTime; }

  // sets
  virtual void setNv12 (uint8_t* nv12) = 0;
  virtual void setYuv420 (void* context, uint8_t** data, int* linesize)  = 0;
  void setYuvRgbTime (int64_t time) { mYuvRgbTime = time; }

protected:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  uint16_t mStride = 0;
  uint32_t* mPixels = nullptr;
  uint32_t mPesSize = 0;
  int64_t mDecodeTime = 0;
  int64_t mYuvRgbTime = 0;
  };
//}}}

//{{{
class cVideoDecoder {
public:
  cVideoDecoder() = default;
  virtual ~cVideoDecoder() = default;

  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          function<void (cFrame* frame)> addFrameCallback) = 0;
  };
//}}}
//{{{
class cMfxVideoDecoder : public cVideoDecoder {
public:
  //{{{
  cMfxVideoDecoder (uint8_t streamType) : cVideoDecoder() {

    cLog::log (LOGINFO, fmt::format ("cMfxVideoDecoder stream:{}", streamType));

    // MFX_IMPL_AUTO MFX_IMPL_HARDWARE MFX_IMPL_SOFTWARE MFX_IMPL_AUTO_ANY MFX_IMPL_VIA_D3D11
    mfxIMPL mfxImpl = MFX_IMPL_AUTO;
    mfxVersion mfxVersion = {{0,1}};

    mfxStatus status = mMfxSession.Init (mfxImpl, &mfxVersion);
    if (status != MFX_ERR_NONE)
      cLog::log (LOGINFO, "session.Init failed " + getMfxStatusString (status));

    // query selected implementation and version
    status = mMfxSession.QueryIMPL (&mfxImpl);
    if (status != MFX_ERR_NONE)
      cLog::log (LOGINFO, "QueryIMPL failed " + getMfxStatusString (status));

    status = mMfxSession.QueryVersion (&mfxVersion);
    if (status != MFX_ERR_NONE)
      cLog::log (LOGINFO, "QueryVersion failed " + getMfxStatusString (status));
    cLog::log (LOGINFO, getMfxInfoString (mfxImpl, mfxVersion));

    mH264 = (streamType == 27) ;
    mCodecId = mH264 ? MFX_CODEC_AVC : MFX_CODEC_MPEG2;
    }
  //}}}
  //{{{
  virtual ~cMfxVideoDecoder() {

    MFXVideoDECODE_Close (mMfxSession);
    mMfxSession.Close();

    for (int i = 0; i < mNumSurfaces; i++)
      delete mSurfaces[i];
    }
  //}}}

  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          function<void (cFrame* frame)> addFrameCallback) final {

    if (!mGotIframe) {
      //{{{  skip until first Iframe
      char frameType = cDvbUtils::getFrameType (pes, pesSize, mH264);
      if (frameType == 'I')
        mGotIframe = true;
      else {
        cLog::log (LOGINFO, fmt::format ("skip:{} {} {} size:{}", 
                                         frameType, getPtsString (pts), getPtsString (dts), pesSize));
        return pts;
        }
      }
      //}}}

    (void)dts;
    mBitstream.Data = pes;
    mBitstream.DataOffset = 0;
    mBitstream.DataLength = pesSize;
    mBitstream.MaxLength = pesSize;
    mBitstream.TimeStamp = pts;

    if (!mNumSurfaces) {
      //{{{  read header, allocate surfaces, init decoder, return on error
      mfxVideoParam mVideoParams;
      memset (&mVideoParams, 0, sizeof (mVideoParams));
      mVideoParams.mfx.CodecId = mCodecId;
      mVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
      mfxStatus status = MFXVideoDECODE_DecodeHeader (mMfxSession, &mBitstream, &mVideoParams);
      if (status != MFX_ERR_NONE) {
        cLog::log (LOGINFO, "MFXVideoDECODE_DecodeHeader failed " + getMfxStatusString (status));
        return pts;
        }
      //}}}
      //{{{  query numSurfaces, return on error
      mfxFrameAllocRequest frameAllocRequest;
      memset (&frameAllocRequest, 0, sizeof (frameAllocRequest));
      status =  MFXVideoDECODE_QueryIOSurf (mMfxSession, &mVideoParams, &frameAllocRequest);
      if ((status != MFX_ERR_NONE) || !frameAllocRequest.NumFrameSuggested) {
        cLog::log (LOGINFO, "MFXVideoDECODE_QueryIOSurf failed " + getMfxStatusString (status));
        return pts;
        }
      //}}}
      //{{{  align to 32 pixel boundaries
      mWidth = (frameAllocRequest.Info.Width+31) & (~(mfxU16)31);
      mHeight = (frameAllocRequest.Info.Height+31) & (~(mfxU16)31);
      mNumSurfaces = frameAllocRequest.NumFrameSuggested;
      mSurfaces = new mfxFrameSurface1*[mNumSurfaces];
      cLog::log (LOGINFO, fmt::format ("mfxDecode surfaces allocated {}x{} {}", mWidth, mHeight, mNumSurfaces));
      //}}}
      //{{{  alloc surfaces in system memory
      for (int i = 0; i < mNumSurfaces; i++) {
        mSurfaces[i] = new mfxFrameSurface1;
        memset (mSurfaces[i], 0, sizeof (mfxFrameSurface1));
        memcpy (&mSurfaces[i]->Info, &mVideoParams.mfx.FrameInfo, sizeof(mfxFrameInfo));

        // allocate NV12 followed by planar u, planar v
        size_t nv12SizeLuma = mWidth * mHeight;
        size_t nv12SizeAll = (nv12SizeLuma * 12) / 8;
        mSurfaces[i]->Data.Y = new mfxU8[nv12SizeAll];
        mSurfaces[i]->Data.U = mSurfaces[i]->Data.Y + nv12SizeLuma;
        mSurfaces[i]->Data.V = nullptr; // NV12 ignores V pointer
        mSurfaces[i]->Data.Pitch = mWidth;
        }
      //}}}
      //{{{  init decoder, return on error
      status = MFXVideoDECODE_Init (mMfxSession, &mVideoParams);
      if (status != MFX_ERR_NONE) {
        cLog::log (LOGINFO, "MFXVideoDECODE_Init failed " + getMfxStatusString (status));
        return pts;
        }
      //}}}
      }

    mfxStatus status = MFX_ERR_NONE;
    while ((status >= MFX_ERR_NONE) || (status == MFX_ERR_MORE_SURFACE)) {
      auto timePoint = chrono::system_clock::now();
      int index = getFreeSurfaceIndex();
      mfxFrameSurface1* surface = nullptr;
      mfxSyncPoint decodeSyncPoint = nullptr;
      status = MFXVideoDECODE_DecodeFrameAsync (mMfxSession, &mBitstream, mSurfaces[index], &surface, &decodeSyncPoint);
      if (status == MFX_ERR_NONE) {
        status = mMfxSession.SyncOperation (decodeSyncPoint, 60000);
        int64_t decodeTime = chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count();

        cVideoFrame* videoFrame = cVideoFrame::createVideoFrame (surface->Data.TimeStamp, 90000/25);
        videoFrame->init (mWidth, mHeight, surface->Data.Pitch, pesSize, decodeTime);
        timePoint = chrono::system_clock::now();
        videoFrame->setNv12 (surface->Data.Y);
        videoFrame->setYuvRgbTime (
          chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count());

        addFrameCallback (videoFrame);

        pts += videoFrame->getPtsDuration();
        }
      }

    return pts;
    }
  //}}}

private:
  //{{{
  int getFreeSurfaceIndex() {

    if (mSurfaces)
      for (mfxU16 surfaceIndex = 0; surfaceIndex < mNumSurfaces; surfaceIndex++)
        if (mSurfaces[surfaceIndex]->Data.Locked == 0)
          return surfaceIndex;

    return MFX_ERR_NOT_FOUND;
    }
  //}}}
  //{{{
  static string getMfxStatusString (mfxStatus status) {

    string statusString;
    switch (status) {
      case   0: statusString = "No error"; break;
      case  -1: statusString = "Unknown error"; break;
      case  -2: statusString = "Null pointer"; break;
      case  -3: statusString = "Unsupported feature/library load error"; break;
      case  -4: statusString = "Could not allocate memory"; break;
      case  -5: statusString = "Insufficient IO buffers"; break;
      case  -6: statusString = "Invalid handle"; break;
      case  -7: statusString = "Memory lock failure"; break;
      case  -8: statusString = "Function called before initialization"; break;
      case  -9: statusString = "Specified object not found"; break;
      case -10: statusString = "More input data expected"; break;
      case -11: statusString = "More output surfaces expected"; break;
      case -12: statusString = "Operation aborted";  break;
      case -13: statusString = "HW device lost";  break;
      case -14: statusString = "Incompatible video parameters" ; break;
      case -15: statusString = "Invalid video parameters";  break;
      case -16: statusString = "Undefined behavior"; break;
      case -17: statusString = "Device operation failure";  break;
      case -18: statusString = "More bitstream data expected";  break;
      case -19: statusString = "Incompatible audio parameters"; break;
      case -20: statusString = "Invalid audio parameters"; break;
      default: statusString = "Error code";
      }

    return fmt::format ("status {} {}", status, statusString);
    }
  //}}}
  //{{{
  static string getMfxInfoString (mfxIMPL mfxImpl, mfxVersion mfxVersion) {

    return fmt::format ("mfxImpl:{:x}{}{}{}{}{}{}{} verMajor:{} verMinor:{}",
                        mfxImpl,
                        ((mfxImpl & 0x0007) == MFX_IMPL_HARDWARE) ? " hw":"",
                        ((mfxImpl & 0x0007) == MFX_IMPL_SOFTWARE) ? " sw":"",
                        ((mfxImpl & 0x0007) == MFX_IMPL_AUTO_ANY) ? " autoAny":"",
                        ((mfxImpl & 0x0300) == MFX_IMPL_VIA_ANY) ? " any":"",
                        ((mfxImpl & 0x0300) == MFX_IMPL_VIA_D3D9) ? " d3d9":"",
                        ((mfxImpl & 0x0300) == MFX_IMPL_VIA_D3D11) ? " d3d11":"",
                        ((mfxImpl & 0x0300) == MFX_IMPL_VIA_VAAPI) ? " vaapi":"",
                        mfxVersion.Major, mfxVersion.Minor);
    }
  //}}}

  MFXVideoSession mMfxSession;
  mfxBitstream mBitstream;

  mfxU32 mCodecId = MFX_CODEC_AVC;
  mfxU16 mWidth = 0;
  mfxU16 mHeight = 0;

  mfxU16 mNumSurfaces = 0;
  mfxFrameSurface1** mSurfaces = nullptr;

  bool mH264 = false;
  bool mGotIframe = false;
  };
//}}}
//{{{
class cFFmpegVideoDecoder : public cVideoDecoder {
public:
  //{{{
  cFFmpegVideoDecoder (uint8_t streamType)
     : cVideoDecoder(),
       mAvCodec (avcodec_find_decoder ((streamType == 27) ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO)) {

    cLog::log (LOGINFO, fmt::format ("cFFmpegVideoDecoder stream:{}", streamType));

    mAvParser = av_parser_init ((streamType == 27) ? AV_CODEC_ID_H264 : AV_CODEC_ID_MPEG2VIDEO);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    avcodec_open2 (mAvContext, mAvCodec, NULL);

    mH264 = (streamType == 27) ;
    }
  //}}}
  //{{{
  virtual ~cFFmpegVideoDecoder() {

    if (mAvContext)
      avcodec_close (mAvContext);
    if (mAvParser)
      av_parser_close (mAvParser);
    if (mSwsContext)
      sws_freeContext (mSwsContext);
    }
  //}}}

  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                  function<void (cFrame* frame)> addFrameCallback) final {

    char frameType = cDvbUtils::getFrameType (pes, pesSize, mH264);
    if (frameType == 'I') {
      //{{{  h264 pts wrong, frames decode in presentation order, correct on Iframe
      if ((mInterpolatedPts >= 0) && (pts != dts))
        cLog::log (LOGERROR, fmt::format ("lost:{} pts:{} dts:{} size:{}",
                                          frameType, getPtsString (pts), getPtsString (dts), pesSize));
      mInterpolatedPts = dts;
      mGotIframe = true;
      }
      //}}}
    if (!mGotIframe) {
      //{{{  skip until first Iframe
      cLog::log (LOGINFO, fmt::format ("skip:{} pts:{} dts:{} size:{}",
                                       frameType, getPtsString (pts), getPtsString (dts), pesSize));
      return pts;
      }
      //}}}

    AVPacket* avPacket = av_packet_alloc();
    AVFrame* avFrame = av_frame_alloc();
    uint8_t* frame = pes;
    uint32_t frameSize = pesSize;
    while (frameSize) {
      auto timePoint = chrono::system_clock::now();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                        frame, (int)frameSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;
          int64_t decodeTime =
            chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count();

          // create new videoFrame
          cVideoFrame* videoFrame = cVideoFrame::createVideoFrame (mInterpolatedPts,
            (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num);
          videoFrame->init (static_cast<uint16_t>(avFrame->width),
                            static_cast<uint16_t>(avFrame->height),
                            static_cast<uint16_t>(avFrame->width),
                            frameSize, decodeTime);
          mInterpolatedPts += videoFrame->getPtsDuration();

          // yuv420 -> rgba
          timePoint = chrono::system_clock::now();
          if (!mSwsContext)
            //{{{  init swsContext with known width,height
            mSwsContext = sws_getContext (videoFrame->getWidth(), videoFrame->getHeight(), AV_PIX_FMT_YUV420P,
                                          videoFrame->getWidth(), videoFrame->getHeight(), AV_PIX_FMT_RGBA,
                                          SWS_BILINEAR, NULL, NULL, NULL);
            //}}}
          videoFrame->setYuv420 (mSwsContext, avFrame->data, avFrame->linesize);
          videoFrame->setYuvRgbTime (
            chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count());
          //av_frame_unref (mAvFrame);

          // add videoFrame
          addFrameCallback (videoFrame);
          }
        }
      frame += bytesUsed;
      frameSize -= bytesUsed;
      }

    av_frame_free (&avFrame);
    av_packet_free (&avPacket);
    return mInterpolatedPts;
    }
  //}}}

private:
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;
  SwsContext* mSwsContext = nullptr;

  bool mH264 = false;
  bool mGotIframe = false;
  int64_t mInterpolatedPts = -1;
  };
//}}}

// cVideoRender
//{{{
cVideoRender::cVideoRender (const std::string name, uint8_t streamType, bool mfxDecoder)
    : cRender(name, streamType), mMaxPoolSize(kVideoPoolSize) {

  if (mfxDecoder)
    mVideoDecoder = new cMfxVideoDecoder (streamType);
  else
    mVideoDecoder = new cFFmpegVideoDecoder (streamType);
  }
//}}}
//{{{
cVideoRender::~cVideoRender() {

  for (auto frame : mFrames)
    delete frame.second;
  mFrames.clear();
  }
//}}}

//{{{
string cVideoRender::getInfoString() const {
  return fmt::format ("{} {}x{} {:5d}:{}", mFrames.size(), mWidth, mHeight, mDecodeTime, mYuvRgbTime);
  }
//}}}
//{{{
cTexture* cVideoRender::getTexture (int64_t playPts, cGraphics& graphics) {

  // locked
  shared_lock<shared_mutex> lock (mSharedMutex);

  if (playPts != mTexturePts) {
    // new pts to display
    uint8_t* pixels = nullptr;

    if (mPtsDuration > 0) {
      auto it = mFrames.find (playPts / mPtsDuration);
      if (it != mFrames.end()) // match found
        pixels =  (*it).second->getPixels();
      }

    if (!pixels) {
     // match notFound, try first
     auto it = mFrames.begin();
      if (it != mFrames.end())
        pixels = (*it).second->getPixels();
      }

    if (pixels) {
      if (mTexture == nullptr) // create
        mTexture = graphics.createTexture ({getWidth(), getHeight()}, pixels);
      else
        mTexture->setPixels (pixels);
      mTexturePts = playPts;
      }
    }

  return mTexture;
  }
//}}}

//{{{
void cVideoRender::addFrame (cVideoFrame* frame) {

  // locked
  unique_lock<shared_mutex> lock (mSharedMutex);

  mFrames.emplace(frame->getPts() / frame->getPtsDuration(), frame);

  if (mFrames.size() >= mMaxPoolSize) {
    // delete youngest frame
    auto it = mFrames.begin();
    auto frameToDelete = (*it).second;
    it = mFrames.erase (it);
    delete frameToDelete;
    }
  }
//}}}
//{{{
void cVideoRender::processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,  bool skip) {

  (void)pts;
  (void)skip;
  //log ("pes", fmt::format ("pts:{} size:{}", getFullPtsString (pts), pesSize));
  //logValue (pts, (float)pesSize);

  mVideoDecoder->decode (pes, pesSize, pts, dts, [&](cFrame* frame) noexcept {
    // addFrame lambda
    cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
    mWidth = videoFrame->getWidth();
    mHeight = videoFrame->getHeight();
    mLastPts = videoFrame->getPts();
    mPtsDuration = videoFrame->getPtsDuration();
    mDecodeTime = videoFrame->getDecodeTime();
    mYuvRgbTime = videoFrame->getYuvRgbTime();

    addFrame (videoFrame);
    logValue (videoFrame->getPts(), (float)mDecodeTime);
    });
  }
//}}}

// cVideoFrame factory classes
//{{{
class cVideoFramePlanarRgba : public cVideoFrame {
public:
  cVideoFramePlanarRgba (int64_t pts, int64_t ptsDuration) : cVideoFrame(pts, ptsDuration) {}
  //{{{
  virtual ~cVideoFramePlanarRgba() {
    #ifdef _WIN32
      _aligned_free (mYbuf);
      _aligned_free (mUbuf);
      _aligned_free (mVbuf);
    #else
      free (mYbuf);
      free (mUbuf);
      free (mVbuf);
    #endif
    }
  //}}}

  //{{{
  virtual void init (uint16_t width, uint16_t height, uint16_t stride, uint32_t pesSize, int64_t decodeTime) final {

    mWidth = width;
    mHeight = height;
    mStride = stride;

    mPesSize = pesSize;
    mDecodeTime = decodeTime;

    mYStride = mStride;
    mUVStride = mStride/2;

    mArgbStride = mWidth;

    #ifdef _WIN32
      if (!mPixels)
        mPixels = (uint32_t*)_aligned_malloc (mWidth * mHeight * 4, 128);
      if (!mYbuf)
        mYbuf = (uint8_t*)_aligned_malloc (mHeight * mYStride * 3 / 2, 128);
      if (!mUbuf)
        mUbuf = (uint8_t*)_aligned_malloc ((mHeight/2) * mUVStride, 128);
      if (!mVbuf)
        mVbuf = (uint8_t*)_aligned_malloc ((mHeight/2) * mUVStride, 128);
    #else
      if (!mPixels)
        mPixels = (uint32_t*)aligned_alloc (128, mWidth * mHeight * 4);
      if (!mYbuf)
        mYbuf = (uint8_t*)aligned_alloc (128, mHeight * mYStride * 3 / 2);
      if (!mUbuf)
        mUbuf = (uint8_t*)aligned_alloc (128, (mHeight/2) * mUVStride);
      if (!mVbuf)
        mVbuf = (uint8_t*)aligned_alloc (128, (mHeight/2) * mUVStride);
    #endif
    }
  //}}}

  //{{{
  virtual void setNv12 (uint8_t* nv12) final {

    // copy y of nv12 to y
    memcpy (mYbuf, nv12, mHeight * mYStride * 3 / 2);

    // copy u and v of nv12 to planar uv
    uint8_t* uv = mYbuf + (mHeight * mYStride);
    uint8_t* u = mUbuf;
    uint8_t* v = mVbuf;
    for (int i = 0; i < (mHeight/2) * mUVStride; i++) {
      *u++ = *uv++;
      *v++ = *uv++;
      }

    // constants
    __m128i ysub  = _mm_set1_epi32 (0x00100010);
    __m128i uvsub = _mm_set1_epi32 (0x00800080);
    __m128i facy  = _mm_set1_epi32 (0x004a004a);
    __m128i facrv = _mm_set1_epi32 (0x00660066);
    __m128i facgu = _mm_set1_epi32 (0x00190019);
    __m128i facgv = _mm_set1_epi32 (0x00340034);
    __m128i facbu = _mm_set1_epi32 (0x00810081);
    __m128i zero  = _mm_set1_epi32 (0x00000000);
    __m128i opaque = _mm_set1_epi32 (0xFFFFFFFF);

    for (uint16_t y = 0; y < mHeight; y += 2) {
      __m128i* srcy128r0 = (__m128i *)(mYbuf + (mYStride * y));
      __m128i* srcy128r1 = (__m128i *)(mYbuf + (mYStride * y) + mYStride);
      __m64* srcu64 = (__m64 *)(mUbuf + mUVStride * (y/2));
      __m64* srcv64 = (__m64 *)(mVbuf + mUVStride * (y/2));

      __m128i* dstrgb128r0 = (__m128i *)(mPixels + (mArgbStride * y));
      __m128i* dstrgb128r1 = (__m128i *)(mPixels + (mArgbStride * y) + mArgbStride);

      for (uint16_t x = 0; x < mWidth; x += 16) {
        __m128i u0 = _mm_loadl_epi64 ((__m128i*)srcu64); srcu64++;
        __m128i v0 = _mm_loadl_epi64 ((__m128i*)srcv64); srcv64++;

        __m128i y0r0 = _mm_load_si128( srcy128r0++ );
        __m128i y0r1 = _mm_load_si128( srcy128r1++ );

        // constant y factors
        __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r0, zero), ysub), facy);
        __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r0, zero), ysub), facy);
        __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (y0r1, zero), ysub), facy);
        __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (y0r1, zero), ysub), facy);

        // expand u and v so they're aligned with y values
        u0 = _mm_unpacklo_epi8 (u0, zero);
        __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (u0, u0), uvsub);
        __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (u0, u0), uvsub);

        v0 = _mm_unpacklo_epi8 (v0,  zero);
        __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (v0, v0), uvsub);
        __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
        __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
        __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
        __m128i bu00 = _mm_mullo_epi16 (facbu, u00);

        __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (v0, v0), uvsub);
        __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
        __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
        __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
        __m128i bu01 = _mm_mullo_epi16 (facbu, u01);

        // row 0
        __m128i r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6);
        __m128i r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6);
        r00 = _mm_packus_epi16 (r00, r01);  // rrrr.. saturated

        __m128i g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6);
        __m128i g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6);
        g00 = _mm_packus_epi16 (g00, g01);  // gggg.. saturated

        __m128i b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6);
        __m128i b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6);
        b00 = _mm_packus_epi16 (b00, b01);  // bbbb.. saturated

        __m128i gbgb = _mm_unpacklo_epi8 (r00, g00);       // gbgb..
        r01 = _mm_unpacklo_epi8 (b00, opaque);             // arar..
        __m128i rgb0123 = _mm_unpacklo_epi16 (gbgb, r01);  // argbargb..
        __m128i rgb4567 = _mm_unpackhi_epi16 (gbgb, r01);  // argbargb..

        gbgb = _mm_unpackhi_epi8 (r00, g00 );
        r01  = _mm_unpackhi_epi8 (b00, opaque);
        __m128i rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        __m128i rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        _mm_stream_si128 (dstrgb128r0++, rgb0123);
        _mm_stream_si128 (dstrgb128r0++, rgb4567);
        _mm_stream_si128 (dstrgb128r0++, rgb89ab);
        _mm_stream_si128 (dstrgb128r0++, rgbcdef);

        // row 1
        r00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6);
        r01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6);
        g00 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6);
        g01 = _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6);
        b00 = _mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6);
        b01 = _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6);

        r00 = _mm_packus_epi16 (r00, r01);  // rrrr.. saturated
        g00 = _mm_packus_epi16 (g00, g01);  // gggg.. saturated
        b00 = _mm_packus_epi16 (b00, b01);  // bbbb.. saturated

        gbgb = _mm_unpacklo_epi8 (r00,  g00);     // gbgb..
        r01  = _mm_unpacklo_epi8 (b00,  opaque);  // arar..
        rgb0123 = _mm_unpacklo_epi16 (gbgb, r01); // argbargb..
        rgb4567 = _mm_unpackhi_epi16 (gbgb, r01); // argbargb..

        gbgb = _mm_unpackhi_epi8 (r00, g00);
        r01  = _mm_unpackhi_epi8 (b00, opaque);
        rgb89ab = _mm_unpacklo_epi16 (gbgb, r01);
        rgbcdef = _mm_unpackhi_epi16 (gbgb, r01);

        _mm_stream_si128 (dstrgb128r1++, rgb0123);
        _mm_stream_si128 (dstrgb128r1++, rgb4567);
        _mm_stream_si128 (dstrgb128r1++, rgb89ab);
        _mm_stream_si128 (dstrgb128r1++, rgbcdef);
        }
      }
    }
  //}}}

  #if defined(INTEL_SSE2)
    //{{{
    virtual void setYuv420 (void* context, uint8_t** data, int* linesize) final {
    // intel intrinsics sse2 convert, fast, but sws is same sort of thing may be a bit faster in the loops
      (void)context;

      uint8_t* yBuffer = data[0];
      uint8_t* uBuffer = data[1];
      uint8_t* vBuffer = data[2];

      int yStride = linesize[0];
      int uvStride = linesize[1];

      __m128i ysub  = _mm_set1_epi32 (0x00100010);
      __m128i uvsub = _mm_set1_epi32 (0x00800080);
      __m128i facy  = _mm_set1_epi32 (0x004a004a);
      __m128i facrv = _mm_set1_epi32 (0x00660066);
      __m128i facgu = _mm_set1_epi32 (0x00190019);
      __m128i facgv = _mm_set1_epi32 (0x00340034);
      __m128i facbu = _mm_set1_epi32 (0x00810081);
      __m128i zero  = _mm_set1_epi32 (0x00000000);
      __m128i alpha = _mm_set1_epi32 (0xFFFFFFFF);

      // dst row pointers
      __m128i* dstrgb128r0 = (__m128i*)mPixels;
      __m128i* dstrgb128r1 = (__m128i*)(mPixels + mWidth);

      for (int y = 0; y < mHeight; y += 2) {
        // calc src row pointers
        __m128i* srcY128r0 = (__m128i*)(yBuffer + yStride*y);
        __m128i* srcY128r1 = (__m128i*)(yBuffer + yStride*y + yStride);
        __m64* srcU64 = (__m64*)(uBuffer + uvStride * (y/2));
        __m64* srcV64 = (__m64*)(vBuffer + uvStride * (y/2));

        for (int x = 0; x < mWidth; x += 16) {
          //{{{  process row
          // row01 u = 0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
          __m128i temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcU64++), zero);
          // row01 u00 = 0.u0 0.u0 0.u1 0.u1 0.u2 0.u2 0.u3 0.u3
          __m128i u00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
          // row01 u01 = 0.u4 0.u4 0.u5 0.u5 0.u6 0.u6 0.u7 0.u7
          __m128i u01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

          // row01 v
          temp = _mm_unpacklo_epi8 (_mm_loadl_epi64 ((__m128i*)srcV64++), zero); // 0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
          __m128i v00 = _mm_sub_epi16 (_mm_unpacklo_epi16 (temp, temp), uvsub);
          __m128i v01 = _mm_sub_epi16 (_mm_unpackhi_epi16 (temp, temp), uvsub);

          // row0
          temp = _mm_load_si128 (srcY128r0++);
          __m128i y00r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
          __m128i y01r0 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

          __m128i rv00 = _mm_mullo_epi16 (facrv, v00);
          __m128i rv01 = _mm_mullo_epi16 (facrv, v01);
          __m128i r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, rv00), 6),
                                          _mm_srai_epi16 (_mm_add_epi16 (y01r0, rv01), 6)); // rrrr.. saturated

          __m128i gu00 = _mm_mullo_epi16 (facgu, u00);
          __m128i gu01 = _mm_mullo_epi16 (facgu, u01);
          __m128i gv00 = _mm_mullo_epi16 (facgv, v00);
          __m128i gv01 = _mm_mullo_epi16 (facgv, v01);
          __m128i g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r0, gu00), gv00), 6),
                                          _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r0, gu01), gv01), 6)); // gggg.. saturated

          __m128i bu00 = _mm_mullo_epi16 (facbu, u00);
          __m128i bu01 = _mm_mullo_epi16 (facbu, u01);
          __m128i b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r0, bu00), 6),
                                          _mm_srai_epi16 (_mm_add_epi16 (y01r0, bu01), 6)); // bbbb.. saturated

          __m128i abab = _mm_unpacklo_epi8 (b00, alpha);
          __m128i grgr = _mm_unpacklo_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

          abab = _mm_unpackhi_epi8 (b00, alpha);
          grgr = _mm_unpackhi_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r0++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r0++, _mm_unpackhi_epi16 (grgr, abab));

          // row1
          temp = _mm_load_si128 (srcY128r1++);
          __m128i y00r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpacklo_epi8 (temp, zero), ysub), facy);
          __m128i y01r1 = _mm_mullo_epi16 (_mm_sub_epi16 (_mm_unpackhi_epi8 (temp, zero), ysub), facy);

          r00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, rv00), 6),
                                  _mm_srai_epi16 (_mm_add_epi16 (y01r1, rv01), 6)); // rrrr.. saturated
          g00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y00r1, gu00), gv00), 6),
                                  _mm_srai_epi16 (_mm_sub_epi16 (_mm_sub_epi16 (y01r1, gu01), gv01), 6)); // gggg.. saturated
          b00 = _mm_packus_epi16 (_mm_srai_epi16 (_mm_add_epi16 (y00r1, bu00), 6),
                                  _mm_srai_epi16 (_mm_add_epi16 (y01r1, bu01), 6)); // bbbb.. saturated

          abab = _mm_unpacklo_epi8 (b00, alpha);
          grgr = _mm_unpacklo_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));

          abab = _mm_unpackhi_epi8 (b00, alpha);
          grgr = _mm_unpackhi_epi8 (r00, g00);
          _mm_stream_si128 (dstrgb128r1++, _mm_unpacklo_epi16 (grgr, abab));
          _mm_stream_si128 (dstrgb128r1++, _mm_unpackhi_epi16 (grgr, abab));
          }
          //}}}

        // skip a dst row
        dstrgb128r0 += mWidth / 4;
        dstrgb128r1 += mWidth / 4;
        }
      }
    //}}}
  #else // ARM NEON
    //{{{
    virtual void setYuv420 (void* context, uint8_t** data, int* linesize) final {
    // ARM NEON covert, maybe slower than sws table lookup

      // constants
      int const heightDiv2 = mHeight / 2;
      int const widthDiv8 = mWidth / 8;

      uint8x8_t const yOffset = vdup_n_u8 (16);
      int16x8_t const uvOffset = vdupq_n_u16 (128);
      int32x4_t const rounding = vdupq_n_s32 (128);

      uint8_t* y0 = (uint8_t*)data[0];
      uint8_t* y1 = (uint8_t*)data[0] + linesize[0];
      uint8_t* u = (uint8_t*)data[1];
      uint8_t* v = (uint8_t*)data[2];
      uint8_t* dst0 = (uint8_t*)mBuffer8888;
      uint8_t* dst1 = (uint8_t*)mBuffer8888 + (mWidth * 4);

      uint8x8x4_t block;
      block.val[3] = vdup_n_u8 (0xFF);

      for (int j = 0; j < heightDiv2; ++j) {
        for (int i = 0; i < widthDiv8; ++i) {
          //{{{  2 rows of 8 pix
          // U load 8 u8 - u01234567 = u0 u1 u2 u3 u4 u5 u6 u7
          uint8x8_t const u01234567 = vld1_u8 (u);
          u += 4;
          // - uu01234567 =  0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7
          int16x8_t const uu01234567 = (int16x8_t)vmovl_u8 (u01234567);
          // - u01234567o = (0.u0 0.u1 0.u2 0.u3 0.u4 0.u5 0.u6 0.u7) - half
          int16x8_t const uu01234567o = vsubq_s16 (uu01234567, uvOffset);
          // - u0123h = 0.u0 0.u1 0.u2 0.u3 - u4567 unused, unwind loop?
          int16x4_t const uu0123o = vget_low_s16 (uu01234567o);

          // V load 8 u8 - v01234567 = v0 v1 v2 v3 v4 v5 v6 v7
          uint8x8_t const v01234567 = vld1_u8 (v);
          v += 4;
          // - vv01234567 =  0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7
          int16x8_t const vv01234567 = (int16x8_t)vmovl_u8 (v01234567);
          // - vv01234567o = (0.v0 0.v1 0.v2 0.v3 0.v4 0.v5 0.v6 0.v7) - half
          int16x8_t const vv01234567o = vsubq_s16 (vv01234567, uvOffset);
          // - vv0123 = 0.v0 0.v1 0.v2 0.v3 - v4567 unused, unwind loop?
          int16x4_t const vv0123o = vget_low_s16 (vv01234567o);

          // r = 128 + 409v
          int32x4_t const r0123os = vmlal_n_s16 (rounding, uu0123o, 409);
          int32x4x2_t const r00112233os = vzipq_s32 (r0123os, r0123os);
          // g = 128 - 100u - 208v
          int32x4_t const g0123os = vmlal_n_s16 (vmlal_n_s16 (rounding, uu0123o, -208), vv0123o, -100);
          int32x4x2_t const g00112233os = vzipq_s32 (g0123os, g0123os);
          // b = 128 + 516u
          int32x4_t const b0123os = vmlal_n_s16 (rounding, vv0123o, 516);
          int32x4x2_t const b00112233os = vzipq_s32 (b0123os, b0123os);

          // row0
          uint8x8_t const row0y01234567 = vld1_u8 (y0);
          y0 += 8;
          uint8x8_t const row0y01234567o = vqsub_u8 (row0y01234567, yOffset);
          uint16x8_t const row0yy01234567o = vmovl_u8 (row0y01234567o);

          int32x4_t const row0y0123os = vmulq_n_u32 (vmovl_u16 (vget_low_u16 (row0yy01234567o)), 298);
          int32x4_t const row0y4567os = vmulq_n_u32 (vmovl_u16 (vget_high_u16 (row0yy01234567o)), 298);

          block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (b00112233os.val[0], row0y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (b00112233os.val[1], row0y4567os))), 8);
          block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (g00112233os.val[0], row0y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (g00112233os.val[1], row0y4567os))), 8),
          block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (r00112233os.val[0], row0y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (r00112233os.val[1], row0y4567os))), 8),
          vst4_u8 (dst0, block);
          dst0 += 8 * 4;

          // row1
          uint8x8_t const row1y01234567 = vld1_u8 (y1);
          y1 += 8;
          uint8x8_t const row1y01234567o = vqsub_u8 (row1y01234567, yOffset);
          uint16x8_t const row1yy01234567o = vmovl_u8 (row1y01234567o);

          int32x4_t const row1y0123os = vmulq_n_u32 (vmovl_u16 (vget_low_u16 (row1yy01234567o)), 298);
          int32x4_t const row1y4567os = vmulq_n_u32 (vmovl_u16 (vget_high_u16 (row1yy01234567o)), 298);

          block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (b00112233os.val[0], row1y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (b00112233os.val[1], row1y4567os))), 8);
          block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (g00112233os.val[0], row1y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (g00112233os.val[1], row1y4567os))), 8),
          block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (r00112233os.val[0], row1y0123os)),
                                                    vqmovun_s32 (vaddq_s32 (r00112233os.val[1], row1y4567os))), 8),
          vst4_u8 (dst1, block);

          dst1 += 8 * 4;
          }
          //}}}
        y0 += linesize[0];
        y1 += linesize[0];
        }
      }
    //}}}
  #endif

private:
  uint16_t mYStride = 0;
  uint16_t mUVStride = 0;
  uint16_t mArgbStride = 0;

  uint8_t* mYbuf = nullptr;
  uint8_t* mUbuf = nullptr;
  uint8_t* mVbuf = nullptr;
  };
//}}}
//{{{
class cVideoFramePlanarRgbaSws : public cVideoFrame {
public:
  cVideoFramePlanarRgbaSws (int64_t pts, int64_t ptsDuration) : cVideoFrame(pts, ptsDuration) {}
  virtual ~cVideoFramePlanarRgbaSws() = default;

  //{{{
  virtual void init (uint16_t width, uint16_t height, uint16_t stride,
                     uint32_t pesSize, int64_t decodeTime) final {

    mWidth = width;
    mHeight = height;
    mStride = stride;

    mPesSize = pesSize;
    mDecodeTime = decodeTime;

    #ifdef _WIN32
      if (!mPixels)
        // allocate aligned buffer
        mPixels = (uint32_t*)_aligned_malloc (width * height * 4, 128);
    #else
      if (!mPixels)
        // allocate aligned buffer
        mPixels = (uint32_t*)aligned_alloc (128, width * height * 4);
    #endif
    }
  //}}}

  virtual void setNv12 (uint8_t* nv12) final {
    (void)nv12;
    cLog::log (LOGERROR, "cVideoFramePlanarRgbaSws setNv12 unimplemented");
    }

  virtual void setYuv420 (void* context, uint8_t** data, int* linesize) final {
    // ffmpeg libswscale convert data to mPixels using swsContext
    uint8_t* dstData[1] = { (uint8_t*)mPixels };
    int dstStride[1] = { mWidth * 4 };
    sws_scale ((SwsContext*)context, data, linesize, 0, mHeight, dstData, dstStride);
    }
  };
//}}}

//{{{
cVideoFrame* cVideoFrame::createVideoFrame (int64_t pts, int64_t ptsDuration) {

  return new cVideoFramePlanarRgba (pts, ptsDuration);
  //return new cVideoFramePlanarRgbaSws(pts, ptsDuration);
  }
//}}}

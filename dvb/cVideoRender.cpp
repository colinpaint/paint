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
#include "cDecoder.h"

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  }

#if defined (__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
  #define INTEL_SSE2
  #define INTEL_SSSE3 1
  #include <emmintrin.h>
  #include <tmmintrin.h>
#else
  #include <arm_neon.h>
  //{{{
  void decode_yuv_neon (uint8_t* dst, uint8_t const* y, uint8_t const* uv, int width, int height, uint8_t fill_alpha) {

    // constants
    int const stride = width * 4;
    int const itHeight = height >> 1;
    int const itWidth = width >> 3;

    uint8x8_t const Yshift = vdup_n_u8 (16);
    int16x8_t const half = vdupq_n_u16 (128);
    int32x4_t const rounding = vdupq_n_s32 (128);

    // tmp variable
    uint16x8_t t;

    // pixel block to temporary store 8 pixels
    uint8x8x4_t block;
    block.val[3] = vdup_n_u8 (fill_alpha);

    for (int j = 0; j < itHeight; ++j, y += width, dst += stride) {
      for (int i = 0; i < itWidth; ++i, y += 8, uv += 8, dst += (8 * 4)) {
         t = vmovl_u8 (vqsub_u8 (vld1_u8 (y), Yshift));
         int32x4_t const Y00 = vmulq_n_u32 (vmovl_u16(vget_low_u16 (t)), 298);
         int32x4_t const Y01 = vmulq_n_u32 (vmovl_u16(vget_high_u16 (t)), 298);

         t = vmovl_u8 (vqsub_u8 (vld1_u8(y+width), Yshift));
         int32x4_t const Y10 = vmulq_n_u32 (vmovl_u16 (vget_low_u16 (t)), 298);
         int32x4_t const Y11 = vmulq_n_u32 (vmovl_u16 (vget_high_u16 (t)), 298);

         // loadvu pack 4 sets of uv into a uint8x8_t, layout : { v0,u0, v1,u1, v2,u2, v3,u3 }
         t = vsubq_s16 ((int16x8_t)vmovl_u8 (vld1_u8 (uv)), half);

         // UV.val[0] : v0, v1, v2, v3, UV.val[1] : u0, u1, u2, u3
         int16x4x2_t const UV = vuzp_s16 (vget_low_s16 (t), vget_high_s16 (t));

         // tR : 128+409V, tG : 128-100U-208V, tB : 128+516U
         int32x4_t const tR = vmlal_n_s16 (rounding, UV.val[0], 409);
         int32x4_t const tG = vmlal_n_s16 (vmlal_n_s16 (rounding, UV.val[0], -208), UV.val[1], -100);
         int32x4_t const tB = vmlal_n_s16 (rounding, UV.val[1], 516);

         int32x4x2_t const R = vzipq_s32 (tR, tR); // [tR0, tR0, tR1, tR1] [ tR2, tR2, tR3, tR3]
         int32x4x2_t const G = vzipq_s32 (tG, tG); // [tG0, tG0, tG1, tG1] [ tG2, tG2, tG3, tG3]
         int32x4x2_t const B = vzipq_s32 (tB, tB); // [tB0, tB0, tB1, tB1] [ tB2, tB2, tB3, tB3]

         // upper 8 pixels
         block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (B.val[0], Y00)),
                                                   vqmovun_s32 (vaddq_s32 (B.val[1], Y01))), 8);
         block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (G.val[0], Y00)),
                                                   vqmovun_s32 (vaddq_s32 (G.val[1], Y01))), 8),
         block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (R.val[0], Y00)),
                                                   vqmovun_s32 (vaddq_s32 (R.val[1], Y01))), 8),
         vst4_u8 (dst, block);

         // lower 8 pixels
         block.val[0] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (B.val[0], Y10)),
                                                   vqmovun_s32 (vaddq_s32 (B.val[1], Y11))), 8);
         block.val[1] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (G.val[0], Y10)),
                                                   vqmovun_s32 (vaddq_s32 (G.val[1], Y11))), 8),
         block.val[2] = vshrn_n_u16 (vcombine_u16 (vqmovun_s32 (vaddq_s32 (R.val[0], Y10)),
                                                   vqmovun_s32 (vaddq_s32 (R.val[1], Y11))), 8),
         vst4_u8 (dst+stride, block);
         }
       }
    }
  //}}}
#endif

using namespace std;
//}}}

constexpr uint32_t kVideoPoolSize = 50;
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

  //{{{
  void init (uint16_t width, uint16_t height, uint32_t pesSize, int64_t decodeTime) {

    mWidth = width;
    mHeight = height;

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

  // gets
  uint16_t getWidth() const {return mWidth; }
  uint16_t getHeight() const {return mHeight; }
  uint8_t* getPixels() const { return (uint8_t*)mPixels; }

  uint32_t getPesSize() const { return mPesSize; }
  int64_t getDecodeTime() const { return mDecodeTime; }
  int64_t getYuv420Time() const { return mYuv420Time; }

  // sets
  virtual void setYuv420 (void* context, uint8_t** data, int* linesize)  = 0;
  void setYuv420Time (int64_t time) { mYuv420Time = time; }

protected:
  uint16_t mWidth = 0;
  uint16_t mHeight = 0;
  uint32_t* mPixels = nullptr;

private:
  uint32_t mPesSize = 0;
  int64_t mDecodeTime = 0;
  int64_t mYuv420Time = 0;
  };
//}}}
//{{{
class cVideoDecoder : public cDecoder {
public:
  //{{{
  cVideoDecoder() : cDecoder() {

    mAvParser = av_parser_init (AV_CODEC_ID_H264);
    mAvCodec = avcodec_find_decoder (AV_CODEC_ID_H264);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    avcodec_open2 (mAvContext, mAvCodec, NULL);

    }
  //}}}
  //{{{
  virtual ~cVideoDecoder() {
    sws_freeContext (mSwsContext);
    }
  //}}}

  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts,
                           function<void (cFrame* frame)> addFrameCallback) final {

    AVPacket mAvPacket;
    av_init_packet (&mAvPacket);

    AVFrame* mAvFrame = nullptr;
    mAvFrame = av_frame_alloc();

    uint8_t* frame = pes;
    uint32_t frameSize = pesSize;
    while (frameSize) {
      auto timePoint = chrono::system_clock::now();

      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &mAvPacket.data, &mAvPacket.size,
                                        frame, (int)frameSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
      if (mAvPacket.size) {
        int ret = avcodec_send_packet (mAvContext, &mAvPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, mAvFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;
          int64_t decodeTime =
            chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count();

          // create new videoFrame
          cVideoFrame* videoFrame = cVideoFrame::createVideoFrame (pts,
            (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num);
          videoFrame->init (static_cast<uint16_t>(mAvFrame->width), static_cast<uint16_t>(mAvFrame->height),
                            frameSize, decodeTime);
          pts += videoFrame->getPtsDuration();

          // yuv420 -> rgba
          timePoint = chrono::system_clock::now();
          if (!mSwsContext)
            //{{{  init swsContext with known width,height
            mSwsContext = sws_getContext (videoFrame->getWidth(), videoFrame->getHeight(), AV_PIX_FMT_YUV420P,
                                          videoFrame->getWidth(), videoFrame->getHeight(), AV_PIX_FMT_RGBA,
                                          SWS_BILINEAR, NULL, NULL, NULL);
            //}}}
          videoFrame->setYuv420 (mSwsContext, mAvFrame->data, mAvFrame->linesize);
          videoFrame->setYuv420Time (
            chrono::duration_cast<chrono::microseconds>(chrono::system_clock::now() - timePoint).count());
          av_frame_unref (mAvFrame);

          // add videoFrame
          addFrameCallback (videoFrame);
          }
        }
      frame += bytesUsed;
      frameSize -= bytesUsed;
      }

    av_frame_free (&mAvFrame);
    return pts;
    }
  //}}}

private:
  SwsContext* mSwsContext = nullptr;
  };
//}}}

// cVideoRender
//{{{
cVideoRender::cVideoRender (const std::string name)
    : cRender(name), mMaxPoolSize(kVideoPoolSize) {

  mDecoder = new cVideoDecoder();
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
  return fmt::format ("{} {}x{} {:5d}:{}", mFrames.size(), mWidth, mHeight, mDecodeTime, mYuv420Time);
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
void cVideoRender::processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) {

  (void)pts;
  (void)skip;
  //log ("pes", fmt::format ("pts:{} size:{}", getFullPtsString (pts), pesSize));
  //logValue (pts, (float)pesSize);

  // ffmpeg h264 pts wrong, decode frames in presentation order, pts is correct on I frames
  char frameType = cDvbUtils::getFrameType (pes, pesSize, true);
  if (frameType == 'I') {
    if ((mGuessPts >= 0) && (mGuessPts != dts))
      cLog::log (LOGERROR, fmt::format ("lost:{} to:{} type:{} {}",
                                        getPtsFramesString (mGuessPts, 1800), getPtsFramesString (dts, 1800),
                                        frameType,pesSize));
    mGuessPts = dts;
    mSeenIFrame = true;
    }

  if (!mSeenIFrame) {
    cLog::log (LOGINFO, fmt::format ("waiting for Iframe {} to:{} type:{} size:{}",
                                     getPtsFramesString (mGuessPts, 1800), getPtsFramesString (dts, 1800),
                                     frameType, pesSize));
    return;
    }

  mGuessPts = mDecoder->decode (pes, pesSize, mGuessPts, [&](cFrame* frame) noexcept {
    // addFrame lambda
    cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
    mWidth = videoFrame->getWidth();
    mHeight = videoFrame->getHeight();
    mPtsDuration = videoFrame->getPtsDuration();
    mDecodeTime = videoFrame->getDecodeTime();
    mYuv420Time = videoFrame->getYuv420Time();

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
  virtual ~cVideoFramePlanarRgba() = default;

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
  };
//}}}
//{{{
class cVideoFramePlanarRgbaSws : public cVideoFrame {
public:
  cVideoFramePlanarRgbaSws (int64_t pts, int64_t ptsDuration) : cVideoFrame(pts, ptsDuration) {}
  virtual ~cVideoFramePlanarRgbaSws() = default;

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

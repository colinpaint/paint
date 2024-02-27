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

// h264
#include "../h264/win32.h"
#include "../h264/h264decoder.h"

// app
#include "../app/cApp.h"
#include "../app/cPlatform.h"
#include "../app/myImgui.h"

using namespace std;
using namespace utils;
//}}}
#define kDump264
constexpr bool kMotionVectors = true;
namespace {
  //{{{
  void ffmpegLogCallback (void* ptr, int level, const char* fmt, va_list vargs) {
    char buffer[256];
    (void)level;
    (void)ptr;
    (void)fmt;
    (void)vargs;

    vsprintf (buffer, fmt, vargs);
    for (int i = 0; i < 256; i++)
      if (buffer[i] == 0xa)
        buffer[i] = 0;

    cLog::log (LOGINFO1, fmt::format ("ffmpeg {}", buffer));
    }
  //}}}
  //{{{
  string getFrameInfo (uint8_t* pes, int64_t pesSize, bool h264) {
  // crude mpeg2/h264 NAL parser
    //{{{
    class cBitstream {
    // used to parse H264 stream to find I frames
    public:
      cBitstream (const uint8_t* buffer, uint32_t bit_len) :
        mDecBuffer(buffer), mDecBufferSize(bit_len), mNumOfBitsInBuffer(0), mBookmarkOn(false) {}

      //{{{
      uint32_t getBits (uint32_t numBits) {

        //{{{
        static const uint32_t msk[33] = {
          0x00000000, 0x00000001, 0x00000003, 0x00000007,
          0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
          0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
          0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
          0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
          0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
          0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
          0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
          0xffffffff
          };
        //}}}

        if (numBits == 0)
          return 0;

        uint32_t retData;
        if (mNumOfBitsInBuffer >= numBits) {  // don't need to read from FILE
          mNumOfBitsInBuffer -= numBits;
          retData = mDecData >> mNumOfBitsInBuffer;
          // wmay - this gets done below...retData &= msk[numBits];
          }
        else {
          uint32_t nbits;
          nbits = numBits - mNumOfBitsInBuffer;
          if (nbits == 32)
            retData = 0;
          else
            retData = mDecData << nbits;

          switch ((nbits - 1) / 8) {
            case 3:
              nbits -= 8;
              if (mDecBufferSize < 8)
                return 0;
              retData |= *mDecBuffer++ << nbits;
              mDecBufferSize -= 8;
              // fall through
            case 2:
              nbits -= 8;
              if (mDecBufferSize < 8)
                return 0;
              retData |= *mDecBuffer++ << nbits;
              mDecBufferSize -= 8;
              // fall through
             case 1:
              nbits -= 8;
              if (mDecBufferSize < 8)
                return 0;
              retData |= *mDecBuffer++ << nbits;
              mDecBufferSize -= 8;
              // fall through
            case 0:
              break;
            }
          if (mDecBufferSize < nbits)
            return 0;

          mDecData = *mDecBuffer++;
          mNumOfBitsInBuffer = min(8u, mDecBufferSize) - nbits;
          mDecBufferSize -= min(8u, mDecBufferSize);
          retData |= (mDecData >> mNumOfBitsInBuffer) & msk[nbits];
          }

        return (retData & msk[numBits]);
        };
      //}}}
      //{{{
      uint32_t getUe() {

        uint32_t bits;
        uint32_t read = 0;
        int bits_left;
        bool done = false;
        bits = 0;

        // we want to read 8 bits at a time - if we don't have 8 bits,
        // read what's left, and shift.  The exp_golomb_bits calc remains the same.
        while (!done) {
          bits_left = bits_remain();
          if (bits_left < 8) {
            read = peekBits (bits_left) << (8 - bits_left);
            done = true;
            }
          else {
            read = peekBits (8);
            if (read == 0) {
              getBits (8);
              bits += 8;
              }
            else
             done = true;
            }
          }

        uint8_t coded = exp_golomb_bits[read];
        getBits (coded);
        bits += coded;

        return getBits (bits + 1) - 1;
        }
      //}}}
      //{{{
      int32_t getSe() {

        uint32_t ret;
        ret = getUe();
        if ((ret & 0x1) == 0) {
          ret >>= 1;
          int32_t temp = 0 - ret;
          return temp;
          }

        return (ret + 1) >> 1;
        }
      //}}}

    private:
      //{{{
      uint32_t peekBits (uint32_t bits) {

        bookmark (true);
        uint32_t ret = getBits (bits);
        bookmark (false);
        return ret;
        }
      //}}}
      //{{{
      const uint8_t exp_golomb_bits[256] = {
        8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0,
        };
      //}}}
      //{{{
      void bookmark (bool on) {

        if (on) {
          mNumOfBitsInBuffer_bookmark = mNumOfBitsInBuffer;
          mDecBuffer_bookmark = mDecBuffer;
          mDecBufferSize_bookmark = mDecBufferSize;
          mBookmarkOn = 1;
          mDecData_bookmark = mDecData;
          }

        else {
          mNumOfBitsInBuffer = mNumOfBitsInBuffer_bookmark;
          mDecBuffer = mDecBuffer_bookmark;
          mDecBufferSize = mDecBufferSize_bookmark;
          mDecData = mDecData_bookmark;
          mBookmarkOn = 0;
          }

        };
      //}}}

      //{{{
      int bits_remain() {
        return mDecBufferSize + mNumOfBitsInBuffer;
        };
      //}}}
      //{{{
      int byte_align() {

        int temp = 0;
        if (mNumOfBitsInBuffer != 0)
          temp = getBits (mNumOfBitsInBuffer);
        else {
          // if we are byte aligned, check for 0x7f value - this will indicate
          // we need to skip those bits
          uint8_t readval = static_cast<uint8_t>(peekBits (8));
          if (readval == 0x7f)
            readval = static_cast<uint8_t>(getBits (8));
          }

        return temp;
        };
      //}}}

      const uint8_t* mDecBuffer;
      uint32_t mDecBufferSize;
      uint32_t mNumOfBitsInBuffer;
      bool mBookmarkOn;

      uint8_t mDecData_bookmark = 0;
      uint8_t mDecData = 0;

      uint32_t mNumOfBitsInBuffer_bookmark = 0;
      const uint8_t* mDecBuffer_bookmark = 0;
      uint32_t mDecBufferSize_bookmark = 0;
      };
    //}}}

    uint8_t* pesEnd = pes + pesSize;

    if (h264) {
      string s;
      char frameType = '?';
      while (pes < pesEnd) {
        //{{{  skip past startcode, find next startcode
        uint8_t* buf = pes;
        int64_t bufSize = pesSize;

        uint32_t startOffset = 0;
        if (!buf[0] && !buf[1]) {
          if (!buf[2] && buf[3] == 1) {
            buf += 4;
            startOffset = 4;
            }
          else if (buf[2] == 1) {
            buf += 3;
            startOffset = 3;
            }
          }

        // find next startCode
        uint32_t offset = startOffset;
        uint32_t nalSize = offset;
        uint32_t val = 0xffffffff;
        while (offset++ < bufSize - 3) {
          val = (val << 8) | *buf++;
          if (val == 0x0000001) {
            nalSize = offset - 4;
            break;
            }
          if ((val & 0x00ffffff) == 0x0000001) {
            nalSize = offset - 3;
            break;
            }

          nalSize = (uint32_t)bufSize;
          }
        //}}}
        if (nalSize > 3) {
          // parse bitStream for NALs
          cBitstream bitstream (buf, (nalSize - startOffset) * 8);

          uint32_t forbidddenZeroBit = bitstream.getBits(1);
          if (forbidddenZeroBit != 0)
            s += '*';
          uint32_t nalRefIdc = bitstream.getBits (2);
          uint32_t nalUnitType = bitstream.getBits (5);

          switch (nalUnitType) {
            //{{{
            case 1: { // nonIdr
              bitstream.getUe();
              int nalSubtype = bitstream.getUe();
              switch (nalSubtype) {
                case 0:
                  frameType = 'P';
                  s += fmt::format ("nonIDR:{}:{}:{} ", nalRefIdc, nalSubtype, frameType);
                  return fmt::format ("{} {}", frameType, s);

                case 1:
                  frameType = 'B';
                  s += fmt::format ("nonIDR:{}:{}:{} ", nalRefIdc, nalSubtype, frameType);
                  return fmt::format ("{} {}", frameType, s);

                case 2:
                  frameType = 'I';
                  s += fmt::format ("nonIDR:{}:{}:{} ", nalRefIdc, nalSubtype, frameType);
                  return fmt::format ("{} {}", frameType, s);

                default:
                  s += fmt::format ("nonIDR:{}:unknownSubType:{}:{} ", nalRefIdc, nalSubtype, frameType);
                  break;
                }

              break;
              }
            //}}}
            //{{{
            case 2:   // parta
              s += "PartA ";
              break;
            //}}}
            //{{{
            case 3:   // partb
              s += "PartB ";
              break;
            //}}}
            //{{{
            case 4:   // partc
              s += "PartC ";
              break;
            //}}}
            //{{{
            case 5: { // idr
              bitstream.getUe();
              int nalSubtype = bitstream.getUe();

              switch (nalSubtype) {
                case 1:
                case 5:
                  frameType = 'P';
                  s += fmt::format ("IDR:{}:{}:{} ", nalRefIdc, nalSubtype, frameType);
                  return fmt::format ("{} {}", frameType, s);

                case 2:
                case 6:
                  frameType =  'B';
                  s += fmt::format ("IDR:{}:{}:{} ", nalRefIdc, nalSubtype, frameType);
                  return fmt::format ("{} {}", frameType, s);

                case 3:
                case 7:
                  frameType =  'I';
                  s += fmt::format ("IDR:{}:{}:{} ", nalRefIdc, nalSubtype, frameType);
                  return fmt::format ("{} {}", frameType, s);

                default:
                  s += fmt::format ("IDR:{}:unknownSubType:{} ", nalRefIdc, nalSubtype);
                  break;
                }

              break;
              }
            //}}}
            //{{{
            case 6:   // sei
              s += "SEI ";
              break;
            //}}}
            //{{{
            case 7:   // sps
              s += fmt::format ("SPS:{} ", nalRefIdc);
              break;
            //}}}
            //{{{
            case 8:   // pps
              s += fmt::format ("PPS:{} ", nalRefIdc);
              break;
            //}}}
            //{{{
            case 9:   // avd
              s += "AVD ";
              break;
            //}}}
            //{{{
            case 10:  // eoSeq
              s += "EOseq";
              break;
            //}}}
            //{{{
            case 11:  // eoStream
              s += "EOstream ";
              break;
            //}}}
            //{{{
            case 12:  // filler
              s += "Fill ";
              break;
            //}}}
            //{{{
            case 13:  // seqext
              s += "SeqExt ";
              break;
            //}}}
            //{{{
            case 14:  // prefix
              s += "PFX ";
              break;
            //}}}
            //{{{
            case 15:  // subsetSps
              s += "SubSPS ";
              break;
            //}}}
            //{{{
            case 19:  // aux
              s += "AUX ";
              break;
            //}}}
            //{{{
            case 20:  // sliceExt
              s += "SliceExt ";
              break;
            //}}}
            //{{{
            case 21:  // sliceExtDepth
              s += "SliceExtDepth ";
              break;
            //}}}
            //{{{
            default:
              s += fmt::format ("nal:{}:{} ", nalUnitType, nalSize);
              break;
            //}}}
            }
          }
        pes += nalSize;
        }
      return fmt::format ("{} {}", frameType, s);
      }
    else {
      //{{{  mpeg2
      while (pes + 6 < pesEnd) {
        // look for pictureHeader 00000100
        if (!pes[0] && !pes[1] && (pes[2] == 0x01) && !pes[3])
          // extract frameType I,B,P
          switch ((pes[5] >> 3) & 0x03) {
            case 1:
              return "I";
            case 2:
              return "P";
            case 3:
              return "B";
            default:
              return "?";
          }
        pes++;
        }

      return "?";
      }
      //}}}
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

    //av_log_set_level (AV_LOG_ERROR);
    av_log_set_level (AV_LOG_VERBOSE);
    av_log_set_callback (ffmpegLogCallback);

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

  virtual void decode (uint8_t* pes, uint32_t pesSize, int64_t pts, const string& frameInfo,
                          function<cFrame*(int64_t pts)> allocFrameCallback,
                          function<void (cFrame* frame)> addFrameCallback) final {

    AVFrame* avFrame = av_frame_alloc();
    AVPacket* avPacket = av_packet_alloc();
    uint8_t* frameData = pes;
    uint32_t frameSize = pesSize;
    while (frameSize) {
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext,
                                        &avPacket->data, &avPacket->size, frameData, (int)frameSize,
                                        pts, AV_NOPTS_VALUE, 0);
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;

          if (frameInfo.front() == 'I')
            mSequentialPts = pts;
          int64_t duration = (kPtsPerSecond * mAvContext->framerate.den) / mAvContext->framerate.num;

          // alloc videoFrame
          cFFmpegVideoFrame* frame = dynamic_cast<cFFmpegVideoFrame*>(allocFrameCallback (mSequentialPts));
          if (frame) {
            frame->set (mSequentialPts, duration, frameSize);
            frame->setAVFrame (avFrame, kMotionVectors);
            frame->setFrameInfo (frameInfo);
            addFrameCallback (frame);
            avFrame = av_frame_alloc();
            }
          mSequentialPts += duration;
          }
        }
      frameData += bytesUsed;
      frameSize -= bytesUsed;
      }
    av_frame_unref (avFrame);
    av_frame_free (&avFrame);
    av_packet_free (&avPacket);
    }

private:
  const bool mH264 = false;
  const string mStreamName;
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;

  int64_t mSequentialPts = -1;
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
  void skipPlay (int64_t skipPts) { (void)skipPts; }
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

    #ifdef kDump264
      #ifdef _WIN32
        FILE* h264File = fopen ("c:/tv/test.264", "wb");
      #else
        FILE* h264File = fopen ("/home/pi/tv/test.264", "wb");
      #endif
    #endif

    thread ([=]() {
      cLog::setThreadName ("anal");

      mTransportStream = new cTransportStream ({"anal", 0, {}, {}}, nullptr,
        //{{{  addService lambda
        [&](cTransportStream::cService& service) noexcept {
          mService = &service;
          createDecoder (service);
          },
        //}}}
        //{{{  pes lambda
        [&](cTransportStream::cService& service, cTransportStream::cPidInfo& pidInfo) noexcept {
          if (pidInfo.getPid() == service.getVideoPid()) {
            string info = getFrameInfo (pidInfo.mBuffer, pidInfo.getBufSize(),
                                        service.getVideoStreamTypeId() == 27);
            uint8_t* buffer = (uint8_t*)malloc (pidInfo.getBufSize());
            memcpy (buffer, pidInfo.mBuffer, pidInfo.getBufSize());
            mPes.emplace_back (cPes (buffer, pidInfo.getBufSize(), pidInfo.getPts(), info));

            #ifdef kDump264
              fwrite (buffer, 1, pidInfo.getBufSize(), h264File);
            #endif
            }
          }
        //}}}
        );

      //{{{  read file
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
      //}}}
      //{{{  report totals
      cLog::log (LOGINFO, fmt::format ("{:4.1f}m took {:3.2f}s",
        mFileSize/1000000.f,
        chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - now).count() / 1000.f));

      cLog::log (LOGINFO, "---------------------------------------------------");
      cLog::log (LOGINFO, fmt::format ("- vid:{:5d} {} to {}",
                                       mPes.size(),
                                       getFullPtsString (mPes.front().mPts),
                                       getFullPtsString (mPes.back().mPts)
                                       ));
      cLog::log (LOGINFO, "---------------------------------------------------");
      //}}}
      delete[] chunk;
      fclose (file);
      #ifdef kDump264
        fclose (h264File);
      #endif

      mDecoder->flush();
      size_t i = skipToI (0);
      i = skipToI (i);
      i = skipToI (i);
      i = skipToI (i);
      while (i < mPes.size()) {
        // decode first frame of gop
        decode (i, mPes[i]);
        i++;
        this_thread::sleep_for (40ms);

        // decode rest of gop
        while (mPes[i].mFrameInfo.front() != 'I') {
          decode (i, mPes[i]);
          i++;
          }
        // skip 8 gop
        for (int j = 0; j < 4; j++)
          i = skipToI (++i);

        while (!mPlaying)
          this_thread::sleep_for (1ms);
        }

      cLog::log (LOGERROR, "exit");
      }).detach();
    }
  //}}}

private:
  static inline const size_t kVideoFrames = 50;
  //{{{
  class cPes {
  public:
    cPes (uint8_t* data, uint32_t size, int64_t pts, const string& frameInfo) :
      mData(data), mSize(size), mPts(pts), mFrameInfo(frameInfo) {}

    uint8_t* mData;
    uint32_t mSize;
    int64_t mPts;
    string mFrameInfo;
    };
  //}}}

  //{{{
  size_t skipToI (size_t i) {

    while (i < mPes.size() && (mPes[i].mFrameInfo.front() != 'I'))
      i++;
    return i;
    }
  //}}}

  //{{{
  void createDecoder (cTransportStream::cService& service) {

    mDecoder = new cVideoDecoder (service.getVideoStreamTypeId() == 27);

    for (size_t i = 0; i < kVideoFrames;  i++)
      mVideoFrames[i] = new cFFmpegVideoFrame();
    }
  //}}}
  //{{{
  void decode (size_t pesIndex, cPes pes) {

    if (mDecoder) {
      cLog::log (LOGINFO, fmt::format ("{:5d} {:5d} decode pesPts:{} {}",
                                       mDecodeSeqNum++,
                                       pesIndex,
                                       getPtsString (pes.mPts),
                                       pes.mFrameInfo));

      mDecoder->decode (pes.mData, pes.mSize, pes.mPts, pes.mFrameInfo,
        // allocFrame lambda
        [&](int64_t pts) noexcept {
          (void)pts;
          mVideoFrameIndex = (mVideoFrameIndex + 1) % kVideoFrames;
          mVideoFrames[mVideoFrameIndex]->releaseResources();
          return mVideoFrames[mVideoFrameIndex];
          },

        // addFrame lambda
        [&](cFrame* frame) noexcept {
          cVideoFrame* videoFrame = dynamic_cast<cVideoFrame*>(frame);
          videoFrame->mTextureDirty = true;
          mVideoFrame = videoFrame;
          cLog::log (LOGINFO, fmt::format ("{:5d}      -----> seqPts:{} {}", mAddSeqNum++,
                                           getPtsString (frame->getPts()), videoFrame->getFrameInfo()));
          });
      }
    else
      cLog::log (LOGERROR, fmt::format ("no decoder"));
    }
  //}}}

  string mFileName;
  size_t mFileSize = 0;

  cTransportStream* mTransportStream = nullptr;
  cTransportStream::cService* mService = nullptr;

  vector <cPes> mPes;

  // playing
  int64_t mPlayPts = -1;
  bool mPlaying = true;

  // videoDecoder
  int mVideoFrameIndex = -1;
  array <cVideoFrame*,kVideoFrames> mVideoFrames = { nullptr };
  cVideoFrame* mVideoFrame = nullptr;
  cDecoder* mDecoder = nullptr;

  size_t mDecodeSeqNum = 0;
  size_t mAddSeqNum = 0;
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
        //{{{  draw frameInfo
        string title = fmt::format ("seqPts:{} {:4d} {:5d} {}",
                                    getPtsString (videoFrame->getPts()),
                                    videoFrame->getPtsDuration(),
                                    videoFrame->getPesSize(),
                                    videoFrame->getFrameInfo()
                                    );

        ImVec2 pos = { ImGui::GetTextLineHeight(), mSize.y - 3 * ImGui::GetTextLineHeight()};
        ImGui::SetCursorPos (pos);
        ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
        ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
        ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
        //}}}
        }
      }

    ImGui::PushFont (testApp.getLargeFont());
    //{{{  title
    string title = testApp.getFilePlayer()->getFileName();
    ImVec2 pos = {ImGui::GetTextLineHeight() * 0.25f, 0.f};
    ImGui::SetCursorPos (pos);
    ImGui::TextColored ({0.f,0.f,0.f,1.f}, title.c_str());
    ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
    ImGui::TextColored ({1.f, 1.f,1.f,1.f}, title.c_str());
    //}}}
    //{{{  playPts
    string ptsString = getPtsString (testApp.getFilePlayer()->getPlayPts());
    pos = ImVec2 (mSize - ImVec2(ImGui::GetTextLineHeight() * 7.f, ImGui::GetTextLineHeight()));
    ImGui::SetCursorPos (pos);
    ImGui::TextColored ({0.f,0.f,0.f,1.f}, ptsString.c_str());
    ImGui::SetCursorPos (pos - ImVec2(2.f,2.f));
    ImGui::TextColored ({1.f,1.f,1.f,1.f}, ptsString.c_str());
    //}}}
    ImGui::PopFont();

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

      //{ false, false,  false, ImGuiKey_UpArrow,    [this,&testApp]{ testApp.getFilePlayer()->skipIframe (-1); }},
      //{ false, false,  false, ImGuiKey_DownArrow,  [this,&testApp]{ testApp.getFilePlayer()->skipIframe (1); }},
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

//{{{
void outputPicList (DecodedPicList* pDecPic, int bOutputAllFrames) {

  DecodedPicList* pPic = pDecPic;

  cLog::log (LOGINFO, fmt::format ("outputPicList {}x{}:{} format:{}:{} stride:{}x{}",
                                   pPic->iWidth * ((pPic->iBitDepth+7)>>3), pPic->iHeight,
                                   pPic->iBitDepth,
                                   pPic->iYUVStorageFormat, pPic->bValid,
                                   pPic->iYBufStride, pPic->iUVBufStride
                                   ));

  if (pPic && (((pPic->iYUVStorageFormat == 2) && pPic->bValid == 3) ||
               ((pPic->iYUVStorageFormat != 2) && pPic->bValid == 1)) ) {
    int iWidth = pPic->iWidth * ((pPic->iBitDepth+7)>>3);
    int iHeight = pPic->iHeight;
    int iStride = pPic->iYBufStride;
    int iWidthUV = (pPic->iYUVFormat != YUV444) ? pPic->iWidth>>1 : pPic->iWidth;
    int iHeightUV = (pPic->iYUVFormat == YUV420) ? pPic->iHeight >> 1 : pPic->iHeight;
    iWidthUV *= ((pPic->iBitDepth + 7) >> 3);
    int iStrideUV = pPic->iUVBufStride;

    do {
      //uint8_t* pbBuf = pPic->pY;
      //for (i = 0; i < iHeight; i++)
      //  write (hFileOutput, pbBuf+i*iStride, iWidth);

      //pbBuf = pPic->pU;
      //for(i=0; i < iHeightUV; i++) {
      //  write( hFileOutput, pbBuf+i*iStrideUV, iWidthUV);

      //pbBuf = pPic->pV;
      //for (i = 0; i < iHeightUV; i++) {
      //  write(hFileOutput, pbBuf+i*iStrideUV, iWidthUV);

      pPic->bValid = 0;
      pPic = pPic->pNext;
      } while (pPic != NULL && pPic->bValid && bOutputAllFrames);
    }
  }
//}}}
//{{{
void h264Decode (const string& fileName) {

  // input params
  InputParameters InputParams;
  memset (&InputParams, 0, sizeof(InputParameters));
  strcpy (InputParams.infile, fileName.c_str());
  InputParams.FileFormat = PAR_OF_ANNEXB;
  InputParams.poc_scale = 2;
  InputParams.intra_profile_deblocking = 1;
  InputParams.poc_gap = 2;
  InputParams.ref_poc_gap = 2;
  InputParams.bDisplayDecParams = 1;
  InputParams.dpb_plus[0] = 1;
  //{{{  optional input params
  //InputParams.ref_offset;
  //InputParams.write_uv;
  //InputParams.silent;
  //InputParams.source;                   //!< source related information
  //InputParams.output;                   //!< output related information
  //InputParams.ProcessInput;
  //InputParams.enable_32_pulldown;
  //InputParams.input_file1;          //!< Input video file1
  //InputParams.input_file2;          //!< Input video file2
  //InputParams.input_file3;          //!< Input video file3
  //InputParams.conceal_mode;
  //InputParams.start_frame;
  //InputParams.stdRange;                         //!< 1 - standard range, 0 - full range
  //InputParams.videoCode;                        //!< 1 - 709, 3 - 601:  See VideoCode in io_tiff.
  //InputParams.export_views;
  //InputParams.iDecFrmNum;
  //}}}

  if (OpenDecoder (&InputParams) != DEC_OPEN_NOERR) {
    cLog::log (LOGERROR, "ppen encoder failed");
    return;
    }

  int iRet = 0;
  do {
    DecodedPicList* pDecPicList;
    iRet = DecodeOneFrame (&pDecPicList);
    if (iRet == DEC_EOS || iRet == DEC_SUCCEED)
      outputPicList (pDecPicList, 0);
    else
      cLog::log (LOGERROR, "decoding process failed");
    } while (iRet == DEC_SUCCEED);

  DecodedPicList* pDecPicList;
  iRet = FinitDecoder (&pDecPicList);
  outputPicList (pDecPicList, 1);
  iRet = CloseDecoder();
  }
//}}}

// main
int main (int numArgs, char* args[]) {

  bool softH264 = false;
  string fileName;
  //{{{  options
  cApp::cOptions* options = new cApp::cOptions();
  for (int i = 1; i < numArgs; i++) {
    string param = args[i];
    if (options->parse (param)) // found cApp option
      ;
    else if (param == "h264")
      softH264 = true;
    else
      fileName = param;
    }
  //}}}

  // log
  cLog::init (LOGINFO);
  cLog::log (LOGNOTICE, fmt::format ("test {} {}", softH264, fileName));

  if (softH264)
    h264Decode (fileName);
  else {
    cTestApp testApp (options, new cTestUI());
    testApp.addFile (fileName);
    testApp.mainUILoop();
    }

  return EXIT_SUCCESS;
  }

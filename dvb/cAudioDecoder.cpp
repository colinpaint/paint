// cAudioDecoder.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

#include "cAudioDecoder.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <shared_mutex>
#include <thread>

#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

using namespace std;
//}}}

enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm } ;
namespace {
  //{{{
  uint8_t* parseFrame (uint8_t* framePtr, uint8_t* frameLast,
                       eAudioFrameType& frameType, int& numChannels, int& sampleRate, int& frameLength) {
  // simple mp3 / aacAdts / aacLatm / wav / id3Tag frame parser

    frameType = eAudioFrameType::eUnknown;
    numChannels = 0;
    sampleRate = 0;
    frameLength = 0;

    while (framePtr + 10 < frameLast) { // enough for largest header start - id3 tagSize
      if ((framePtr[0] == 0x56) && ((framePtr[1] & 0xE0) == 0xE0)) {
        //{{{  aacLatm syncWord (0x02b7 << 5) found
        frameType = eAudioFrameType::eAacLatm;

        // guess sampleRate and numChannels
        sampleRate = 48000;
        numChannels = 2;

        frameLength = 3 + (((framePtr[1] & 0x1F) << 8) | framePtr[2]);

        // check for enough bytes for frame body
        return (framePtr + frameLength <= frameLast) ? framePtr : nullptr;
        }
        //}}}
      else if ((framePtr[0] == 0xFF) && ((framePtr[1] & 0xF0) == 0xF0)) {
        // syncWord found
        if ((framePtr[1] & 0x06) == 0) {
          //{{{  got aacAdts header
          // Header consists of 7 or 9 bytes (without or with CRC).
          // AAAAAAAA AAAABCCD EEFFFFGH HHIJKLMM MMMMMMMM MMMOOOOO OOOOOOPP (QQQQQQQQ QQQQQQQQ)
          //
          // A 12  syncword 0xFFF, all bits must be 1
          // B  1  MPEG Version: 0 for MPEG-4, 1 for MPEG-2
          // C  2  Layer: always 0
          // D  1  protection absent, Warning, set to 1 if there is no CRC and 0 if there is CRC
          // E  2  profile, the MPEG-4 Audio Object Type minus 1
          // F  4  MPEG-4 Sampling Frequency Index (15 is forbidden)
          // G  1  private bit, guaranteed never to be used by MPEG, set to 0 when encoding, ignore when decoding
          // H  3  MPEG-4 Channel Configuration (in the case of 0, the channel configuration is sent via an inband PCE)
          // I  1  originality, set to 0 when encoding, ignore when decoding
          // J  1  home, set to 0 when encoding, ignore when decoding
          // K  1  copyrighted id bit, the next bit of a centrally registered copyright identifier, set to 0 when encoding, ignore when decoding
          // L  1  copyright id start, signals that this frame's copyright id bit is the first bit of the copyright id, set to 0 when encoding, ignore when decoding
          // M 13  frame length, this value must include 7 or 9 bytes of header length: FrameLength = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame)
          // O 11  buffer fullness
          // P  2  Number of AAC frames (RDBs) in ADTS frame minus 1, for maximum compatibility always use 1 AAC frame per ADTS frame
          // Q 16  CRC if protection absent is 0

          frameType = eAudioFrameType::eAacAdts;

          const int sampleRates[16] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0,0,0};
          sampleRate = sampleRates [(framePtr[2] & 0x3c) >> 2];

          // guess numChannels
          numChannels = 2;

          // return aacFrame & size
          frameLength = ((framePtr[3] & 0x3) << 11) | (framePtr[4] << 3) | (framePtr[5] >> 5);

          // check for enough bytes for frame body
          return (framePtr + frameLength <= frameLast) ? framePtr : nullptr;
          }
          //}}}
        else {
          //{{{  got mp3 header
          // AAAAAAAA AAABBCCD EEEEFFGH IIJJKLMM
          //{{{  A 31:21  Frame sync (all bits must be set)
          //}}}
          //{{{  B 20:19  MPEG Audio version ID
          //   0 - MPEG Version 2.5 (later extension of MPEG 2)
          //   01 - reserved
          //   10 - MPEG Version 2 (ISO/IEC 13818-3)
          //   11 - MPEG Version 1 (ISO/IEC 11172-3)
          //   Note: MPEG Version 2.5 was added lately to the MPEG 2 standard.
          // It is an extension used for very low bitrate files, allowing the use of lower sampling frequencies.
          // If your decoder does not support this extension, it is recommended for you to use 12 bits for synchronization
          // instead of 11 bits.
          //}}}
          //{{{  C 18:17  Layer description
          //   00 - reserved
          //   01 - Layer III
          //   10 - Layer II
          //   11 - Layer I
          //}}}
          //{{{  D    16  Protection bit
          //   0 - Protected by CRC (16bit CRC follows header)
          //   1 - Not protected
          //}}}
          //{{{  E 15:12  Bitrate index
          // V1 - MPEG Version 1
          // V2 - MPEG Version 2 and Version 2.5
          // L1 - Layer I
          // L2 - Layer II
          // L3 - Layer III
          //   bits  V1,L1   V1,L2  V1,L3  V2,L1  V2,L2L3
          //   0000   free    free   free   free   free
          //   0001  32  32  32  32  8
          //   0010  64  48  40  48  16
          //   0011  96  56  48  56  24
          //   0100  128 64  56  64  32
          //   0101  160 80  64  80  40
          //   0110  192 96  80  96  48
          //   0111  224 112 96  112 56
          //   1000  256 128 112 128 64
          //   1001  288 160 128 144 80
          //   1010  320 192 160 160 96
          //   1011  352 224 192 176 112
          //   1100  384 256 224 192 128
          //   1101  416 320 256 224 144
          //   1110  448 384 320 256 160
          //   1111  bad bad bad bad bad
          //   values are in kbps
          //}}}
          //{{{  F 11:10  Sampling rate index
          //   bits  MPEG1     MPEG2    MPEG2.5
          //    00  44100 Hz  22050 Hz  11025 Hz
          //    01  48000 Hz  24000 Hz  12000 Hz
          //    10  32000 Hz  16000 Hz  8000 Hz
          //    11  reserv.   reserv.   reserv.
          //}}}
          //{{{  G     9  Padding bit
          //   0 - frame is not padded
          //   1 - frame is padded with one extra slot
          //   Padding is used to exactly fit the bitrate.
          // As an example: 128kbps 44.1kHz layer II uses a lot of 418 bytes
          // and some of 417 bytes long frames to get the exact 128k bitrate.
          // For Layer I slot is 32 bits long, for Layer II and Layer III slot is 8 bits long.
          //}}}
          //{{{  H     8  Private bit. This one is only informative.
          //}}}
          //{{{  I   7:6  Channel Mode
          //   00 - Stereo
          //   01 - Joint stereo (Stereo)
          //   10 - Dual channel (2 mono channels)
          //   11 - Single channel (Mono)
          //}}}
          //{{{  K     3  Copyright
          //   0 - Audio is not copyrighted
          //   1 - Audio is copyrighted
          //}}}
          //{{{  L     2  Original
          //   0 - Copy of original media
          //   1 - Original media
          //}}}
          //{{{  M   1:0  emphasis
          //   00 - none
          //   01 - 50/15 ms
          //   10 - reserved
          //   11 - CCIT J.17
          //}}}

          frameType = eAudioFrameType::eMp3;

          //uint8_t version = (framePtr[1] & 0x08) >> 3;
          uint8_t layer = (framePtr[1] & 0x06) >> 1;
          int scale = (layer == 3) ? 48 : 144;

          //uint8_t errp = framePtr[1] & 0x01;

          const uint32_t bitRates[16] = {     0,  32000,  40000,  48000,  56000,  64000,  80000, 96000,
                                         112000, 128000, 160000, 192000, 224000, 256000, 320000,     0 };
          uint8_t bitrateIndex = (framePtr[2] & 0xf0) >> 4;
          uint32_t bitrate = bitRates[bitrateIndex];

          const int sampleRates[4] = { 44100, 48000, 32000, 0};
          uint8_t sampleRateIndex = (framePtr[2] & 0x0c) >> 2;
          sampleRate = sampleRates[sampleRateIndex];

          uint8_t pad = (framePtr[2] & 0x02) >> 1;

          frameLength = (bitrate * scale) / sampleRate;
          if (pad)
            frameLength++;

          //uint8_t priv = (framePtr[2] & 0x01);
          //uint8_t mode = (framePtr[3] & 0xc0) >> 6;
          numChannels = 2;  // guess
          //uint8_t modex = (framePtr[3] & 0x30) >> 4;
          //uint8_t copyright = (framePtr[3] & 0x08) >> 3;
          //uint8_t original = (framePtr[3] & 0x04) >> 2;
          //uint8_t emphasis = (framePtr[3] & 0x03);

          // check for enough bytes for frame body
          return (framePtr + frameLength <= frameLast) ? framePtr : nullptr;
          }
          //}}}
        }
      else if (framePtr[0] == 'R' && framePtr[1] == 'I' && framePtr[2] == 'F' && framePtr[3] == 'F') {
        //{{{  got wav header, dumb but explicit parser
        framePtr += 4;

        //uint32_t riffSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
        framePtr += 4;

        if ((framePtr[0] == 'W') && (framePtr[1] == 'A') && (framePtr[2] == 'V') && (framePtr[3] == 'E')) {
          framePtr += 4;

          if ((framePtr[0] == 'f') && (framePtr[1] == 'm') && (framePtr[2] == 't') && (framePtr[3] == ' ')) {
            framePtr += 4;

            uint32_t fmtSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
            framePtr += 4;

            //uint16_t audioFormat = framePtr[0] + (framePtr[1] << 8);
            numChannels = framePtr[2] + (framePtr[3] << 8);
            sampleRate = framePtr[4] + (framePtr[5] << 8) + (framePtr[6] << 16) + (framePtr[7] << 24);
            //int blockAlign = framePtr[12] + (framePtr[13] << 8);
            framePtr += fmtSize;

            while (!((framePtr[0] == 'd') && (framePtr[1] == 'a') && (framePtr[2] == 't') && (framePtr[3] == 'a'))) {
              framePtr += 4;

              uint32_t chunkSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
              framePtr += 4 + chunkSize;
              }
            framePtr += 4;

            //uint32_t dataSize = framePtr[0] + (framePtr[1] << 8) + (framePtr[2] << 16) + (framePtr[3] << 24);
            framePtr += 4;

            frameType = eAudioFrameType::eWav;
            frameLength = int(frameLast - framePtr);

            // check for some bytes for frame body
            return framePtr < frameLast ? framePtr : nullptr;
            }
          }

        return nullptr;
        }
        //}}}
      else if (framePtr[0] == 'I' && framePtr[1] == 'D' && framePtr[2] == '3') {
        //{{{  got id3 header
        frameType = eAudioFrameType::eId3Tag;

        // return tag & size
        auto tagSize = (framePtr[6] << 21) | (framePtr[7] << 14) | (framePtr[8] << 7) | framePtr[9];
        frameLength = 10 + tagSize;

        // check for enough bytes for frame body
        return (framePtr + frameLength <= frameLast) ? framePtr : nullptr;
        }
        //}}}
      else
        framePtr++;
      }

    return nullptr;
    }
  //}}}
  //{{{
  uint8_t* parseFrame (uint8_t* framePtr, uint8_t* frameLast, int& frameLength) {

    eAudioFrameType frameType = eAudioFrameType::eUnknown;
    int sampleRate = 0;
    int numChannels = 0;
    frameLength = 0;
    framePtr = parseFrame (framePtr, frameLast, frameType, numChannels, sampleRate, frameLength);

    while (framePtr && (frameType == eAudioFrameType::eId3Tag)) {
      // skip id3 frames
      framePtr += frameLength;
      framePtr = parseFrame (framePtr, frameLast, frameType, numChannels, sampleRate, frameLength);
      }

    return framePtr;
    }
  //}}}
  }

//{{{
class cAudioFrame {
public:
  //{{{
  cAudioFrame (int numChannels, int64_t pts, float* samples) : mSamples(samples), mPts(pts) {

    mPowerValues = (float*)malloc (numChannels * 4);
    memset (mPowerValues, 0, numChannels * 4);

    mPeakValues = (float*)malloc (numChannels * 4);
    memset (mPeakValues, 0, numChannels * 4);
    }
  //}}}
  //{{{
  virtual ~cAudioFrame() {
    free (mSamples);
    free (mPowerValues);
    free (mPeakValues);
    }
  //}}}

  // gets
  int64_t getPts() const { return mPts; }
  float* getSamples() const { return mSamples; }
  float* getPowerValues() const { return mPowerValues;  }
  float* getPeakValues() const { return mPeakValues;  }

  // vars
  int64_t mPts;
  float* mSamples;
  float* mPowerValues;
  float* mPeakValues;
  };
//}}}
//{{{  class cAudioFrames
constexpr float kMinPowerValue = 0.25f;
constexpr float kMinPeakValue = 0.25f;
constexpr uint32_t kMaxNumChannels = 2;             // arbitrary chan max
constexpr uint32_t kMaxNumSamplesPerFrame = 2048;   // arbitrary frame max

class cAudioFrames {
public:
  //{{{
  cAudioFrames(eAudioFrameType frameType, int numChannels, int sampleRate, int samplesPerFrame, int maxMapSize)
    : mFrameType(frameType), mNumChannels(numChannels),
      mSampleRate(sampleRate), mSamplesPerFrame(samplesPerFrame),
      mMaxMapSize(maxMapSize) {}
  //}}}
  //{{{
  ~cAudioFrames() {

    unique_lock<shared_mutex> lock (mSharedMutex);

    for (auto frame : mFrameMap)
      delete (frame.second);
    mFrameMap.clear();
    }
  //}}}

  //{{{  get
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }

  eAudioFrameType getFrameType() { return mFrameType; }
  uint32_t getNumChannels() const { return mNumChannels; }
  uint32_t getNumSampleBytes() const { return mNumChannels * sizeof(float); }
  uint32_t getSampleRate() const { return mSampleRate; }
  uint32_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  int64_t getFramesFromSeconds (int64_t seconds) const { return (seconds * mSampleRate) / mSamplesPerFrame; }
  int64_t getSecondsFromFrames (int64_t frames) const { return (frames * mSamplesPerFrame) / mSampleRate; }

  virtual int64_t getFramePtsDuration() const { return 1; }
  virtual int64_t getFrameNumFromPts (int64_t pts) const { return pts; }
  virtual int64_t getPtsFromFrameNum (int64_t frameNum) const { return frameNum; }
  int64_t getPlayPts() const { return mPlayPts; }
  bool getPlaying() const { return mPlaying; }

  // get frameNum
  int64_t getPlayFrameNum() const { return getFrameNumFromPts (mPlayPts); }
  int64_t getFirstFrameNum() const { return mFrameMap.empty() ? 0 : mFrameMap.begin()->first; }
  int64_t getLastFrameNum() const { return mFrameMap.empty() ? 0 : mFrameMap.rbegin()->first;  }
  //{{{
  int64_t getNumFrames() const {
    return mFrameMap.empty() ? 0 : (mFrameMap.rbegin()->first - mFrameMap.begin()->first+1);
    }
  //}}}

  bool getPlayFinished() const;
  std::string getFirstTimeString (int daylightSeconds) const;
  std::string getPlayTimeString (int daylightSeconds) const;
  std::string getLastTimeString (int daylightSeconds) const;

  // get max nums for early allocations
  int getMaxNumSamplesPerFrame() const { return kMaxNumSamplesPerFrame; }
  int getMaxNumSampleBytes() const { return kMaxNumChannels * sizeof(float); }
  int getMaxNumFrameSamplesBytes()const { return getMaxNumSamplesPerFrame() * getMaxNumSampleBytes(); }

  // get max values for ui
  float getMaxPowerValue()  const{ return mMaxPowerValue; }
  float getMaxPeakValue() const { return mMaxPeakValue; }
  //}}}
  //{{{  find
  cAudioFrame* findFrameByFrameNum (int64_t frameNum) const {
    auto it = mFrameMap.find (frameNum);
    return (it == mFrameMap.end()) ? nullptr : it->second;
    }

  cAudioFrame* findFrameByPts (int64_t pts) const { return findFrameByFrameNum (pts); }
  cAudioFrame* findPlayFrame() const { return findFrameByFrameNum (mPlayPts); }
  //}}}
  //{{{  play
  void togglePlaying() { mPlaying = !mPlaying; }

  void setPlayPts (int64_t pts);
  void setPlayFirstFrame();
  void setPlayLastFrame();

  void nextPlayFrame();
  void incPlaySec (int seconds);
  //}}}

  cAudioFrame& addFrame (int64_t pts, float* samples);

  // vars
  const eAudioFrameType mFrameType;
  const int mNumChannels;
  const int mSampleRate = 0;
  const int mSamplesPerFrame = 0;
  const int mMaxMapSize;

  std::shared_mutex mSharedMutex;
  std::map <int64_t, cAudioFrame*> mFrameMap;

  int64_t mPlayPts = 0;

private:
  bool mPlaying = false;

  float mMaxPowerValue = 0.f;
  float mMaxPeakValue = 0.f;
  float mMaxFreqValue = 0.f;
  };

// cAudioFrames members
//{{{
bool cAudioFrames::getPlayFinished() const {
  return mPlayPts > getLastFrameNum();
  }
//}}}
//{{{
string cAudioFrames::getFirstTimeString (int daylightSeconds) const {

  (void)daylightSeconds;

  // scale firstFrameNum as seconds*100
  int64_t value = getSecondsFromFrames (getFirstFrameNum() * 100);
  if (!value)
    return "";

  return getTimeString (getSecondsFromFrames (value), 0);
  }
//}}}
//{{{
string cAudioFrames::getPlayTimeString (int daylightSeconds) const {
// scale pts = frameNum as seconds*100

  return getTimeString (getSecondsFromFrames (getPlayPts() * 100), daylightSeconds);
  }
//}}}
//{{{
string cAudioFrames::getLastTimeString (int daylightSeconds) const {

  (void)daylightSeconds;

  // scale frameNum as seconds*100
  return getTimeString (getSecondsFromFrames (getLastFrameNum() * 100), 0);
  }
//}}}

//{{{
void cAudioFrames::setPlayPts (int64_t pts) {
  mPlayPts = min (pts, getLastFrameNum() + 1);
  }
//}}}
//{{{
void cAudioFrames::setPlayFirstFrame() {
  setPlayPts (getFirstFrameNum());
  }
//}}}
//{{{
void cAudioFrames::setPlayLastFrame() {
  setPlayPts (getLastFrameNum());
  }
//}}}
//{{{
void cAudioFrames::nextPlayFrame() {

  int64_t playPts = mPlayPts + getFramePtsDuration();

  setPlayPts (playPts);
  }
//}}}
//{{{
void cAudioFrames::incPlaySec (int seconds) {
  setPlayPts (max (int64_t(0), mPlayPts + (getFramesFromSeconds (seconds) * getFramePtsDuration())));
  }
//}}}

//{{{
cAudioFrame& cAudioFrames::addFrame (int64_t pts, float* samples) {

  cAudioFrame* frame;
  if (mMaxMapSize && (int(mFrameMap.size()) > mMaxMapSize)) {
    // reuse cAudioFrame

      { // remove frame from map begin with lock
      unique_lock<shared_mutex> lock (mSharedMutex);
      auto it = mFrameMap.begin();
      frame = (*it).second;
      mFrameMap.erase (it);
      }

    //  reuse power,peak,fft buffers, but free samples if we own them
    free (frame->mSamples);
    frame->mSamples = samples;
    frame->mPts = pts;
    }

  else
    // allocate new cAudioFrame
    frame = new cAudioFrame (mNumChannels, pts, samples);

  // calc power,peak
  for (auto channel = 0; channel < mNumChannels; channel++) {
    //{{{  init
    frame->mPowerValues[channel] = 0.f;
    frame->mPeakValues[channel] = 0.f;
    }
    //}}}
  auto samplePtr = samples;
  for (int sample = 0; sample < mSamplesPerFrame; sample++) {
    //{{{  acumulate
    for (auto channel = 0; channel < mNumChannels; channel++) {
      auto value = *samplePtr++;
      frame->mPowerValues[channel] += value * value;
      frame->mPeakValues[channel] = max (abs(frame->mPeakValues[channel]), value);
      }
    }
    //}}}
  for (auto channel = 0; channel < mNumChannels; channel++) {
    //{{{  scale
    frame->mPowerValues[channel] = sqrtf (frame->mPowerValues[channel] / mSamplesPerFrame);
    mMaxPowerValue = max (mMaxPowerValue, frame->mPowerValues[channel]);
    mMaxPeakValue = max (mMaxPeakValue, frame->mPeakValues[channel]);
    }
    //}}}

    { // insert cAudioFrame with lock
    unique_lock<shared_mutex> lock (mSharedMutex);
    mFrameMap.insert (map<int64_t,cAudioFrame*>::value_type (pts / getFramePtsDuration(), frame));
    }

  return *frame;
  }
//}}}
//}}}

//{{{
class cFFmpegAudioDecoder {
public:
  cFFmpegAudioDecoder (eAudioFrameType frameType);
  ~cFFmpegAudioDecoder();

  int32_t getNumChannels() { return mChannels; }
  int32_t getSampleRate() { return mSampleRate; }
  int32_t getNumSamplesPerFrame() { return mSamplesPerFrame; }

  float* decodeFrame (const uint8_t* framePtr, int frameLen, int64_t pts);

private:
  int32_t mChannels = 0;
  int32_t mSampleRate = 0;
  int32_t mSamplesPerFrame = 0;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;

  int64_t mLastPts = -1;
  };

// cFFmpegAudioDecoder members
//{{{
cFFmpegAudioDecoder::cFFmpegAudioDecoder (eAudioFrameType frameType) {

  AVCodecID streamType;

  switch (frameType) {
    case eAudioFrameType::eMp3:
      streamType =  AV_CODEC_ID_MP3;
      break;

    case eAudioFrameType::eAacAdts:
      streamType =  AV_CODEC_ID_AAC;
      break;

    case eAudioFrameType::eAacLatm:
      streamType =  AV_CODEC_ID_AAC_LATM;
      break;

    default:
      cLog::log (LOGERROR, "unknown cFFmpegAacDecoder frameType %d", frameType);
      return;
    }

  mAvParser = av_parser_init (streamType);
  mAvCodec = avcodec_find_decoder (streamType);
  mAvContext = avcodec_alloc_context3 (mAvCodec);
  avcodec_open2 (mAvContext, mAvCodec, NULL);
  }
//}}}
//{{{
cFFmpegAudioDecoder::~cFFmpegAudioDecoder() {

  if (mAvContext)
    avcodec_close (mAvContext);
  if (mAvParser)
    av_parser_close (mAvParser);
  }
//}}}

//{{{
float* cFFmpegAudioDecoder::decodeFrame (const uint8_t* framePtr, int frameLen, int64_t pts) {

  float* outBuffer = nullptr;

  //if (pts != mLastPts + duration) // skip

  AVPacket avPacket;
  av_init_packet (&avPacket);
  auto avFrame = av_frame_alloc();

  auto pesPtr = framePtr;
  auto pesSize = frameLen;
  while (pesSize) {
    auto bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket.data, &avPacket.size,
                                       pesPtr, (int)pesSize, pts, AV_NOPTS_VALUE, 0);
    avPacket.pts = pts;
    pesPtr += bytesUsed;
    pesSize -= bytesUsed;
    if (avPacket.size) {
      auto ret = avcodec_send_packet (mAvContext, &avPacket);
      while (ret >= 0) {
        ret = avcodec_receive_frame (mAvContext, avFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0)
          break;

        if (avFrame->nb_samples > 0) {
          switch (mAvContext->sample_fmt) {
            //{{{
            case AV_SAMPLE_FMT_FLTP: { // 32bit float planar, copy to interleaved, mix down 5.1

              mChannels = 2;
              mSampleRate = avFrame->sample_rate;
              mSamplesPerFrame = avFrame->nb_samples;

              outBuffer = (float*)malloc (mChannels * mSamplesPerFrame * sizeof(float));
              float* dstPtr = outBuffer;

              float* srcPtr0 = (float*)avFrame->data[0];
              float* srcPtr1 = (float*)avFrame->data[1];
              if (avFrame->channels == 6) { // 5.1
                float* srcPtr2 = (float*)avFrame->data[2];
                float* srcPtr3 = (float*)avFrame->data[3];
                float* srcPtr4 = (float*)avFrame->data[4];
                float* srcPtr5 = (float*)avFrame->data[5];
                for (auto sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++ + *srcPtr2++ + *srcPtr4 + *srcPtr5; // left loud
                  *dstPtr++ = *srcPtr1++ + *srcPtr3++ + *srcPtr4++ + *srcPtr5++; // right loud
                  }
                }
              else // stereo
                for (auto sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++;
                  *dstPtr++ = *srcPtr1++;
                  }
                }
              break;
            //}}}
            //{{{
            case AV_SAMPLE_FMT_S16P: // 16bit signed planar, copy scale and copy to interleaved
              mChannels =  avFrame->channels;
              mSampleRate = avFrame->sample_rate;
              mSamplesPerFrame = avFrame->nb_samples;
              outBuffer = (float*)malloc (avFrame->channels * avFrame->nb_samples * sizeof(float));

              for (auto channel = 0; channel < avFrame->channels; channel++) {
                auto dstPtr = outBuffer + channel;
                auto srcPtr = (short*)avFrame->data[channel];
                for (auto sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr = *srcPtr++ / (float)0x8000;
                  dstPtr += mChannels;
                  }
                }
              break;
            //}}}
            default:;
            }

          }
        av_frame_unref (avFrame);
        }
      }
    }

  mLastPts = pts;

  av_frame_free (&avFrame);
  return outBuffer;
  }
//}}}
//}}}
//{{{
class cAudioPlayer {
public:
  cAudioPlayer (cAudioFrames* audioFrames, bool streaming);
  ~cAudioPlayer() {}

  void exit() { mExit = true; }
  void wait();

private:
  bool mExit = false;
  bool mRunning = true;
  };

#ifdef _WIN32
  cAudioPlayer::cAudioPlayer (cAudioFrames* audioFrames, bool streaming) {
    thread playerThread = thread ([=]() {
      // player lambda
      cLog::setThreadName ("play");
      SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
      array <float,2048*2> silence = { 0.f };
      array <float,2048*2> samples = { 0.f };

      audioFrames->togglePlaying();
      //{{{  WSAPI player thread, video follows playPts
      auto device = getDefaultAudioOutputDevice();
      if (device) {
        cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", audioFrames->getSampleRate());
        device->setSampleRate (audioFrames->getSampleRate());
        device->start();

        cAudioFrame* frame;
        while (!mExit) {
          device->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
            // loadSrcSamples callback lambda
            shared_lock<shared_mutex> lock (audioFrames->getSharedMutex());

            frame = audioFrames->findPlayFrame();
            if (audioFrames->getPlaying() && frame && frame->getSamples()) {
              if (audioFrames->getNumChannels() == 1) {
                // audioFrames to stereo
                float* src = frame->getSamples();
                float* dst = samples.data();
                for (uint32_t i = 0; i < audioFrames->getSamplesPerFrame(); i++) {
                  *dst++ = *src;
                  *dst++ = *src++;
                  }
                }
              else
                memcpy (samples.data(), frame->getSamples(), audioFrames->getSamplesPerFrame() * audioFrames->getNumChannels() * sizeof(float));
              srcSamples = samples.data();
              }
            else
              srcSamples = silence.data();
            numSrcSamples = audioFrames->getSamplesPerFrame();

            if (frame && audioFrames->getPlaying())
              audioFrames->nextPlayFrame();
            });

          if (!streaming && audioFrames->getPlayFinished())
            break;
          }

        device->stop();
        }
      //}}}
      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });
    playerThread.detach();
    }

#else
  cAudioPlayer::cAudioPlayer (cAudioFrames* audioFrames, bool streaming) {
    thread playerThread = thread ([=, this]() {
      // player lambda
      cLog::setThreadName ("play");
      array <float,2048*2> silence = { 0.f };
      array <float,2048*2> samples = { 0.f };

      audioFrames->togglePlaying();
      //{{{  audio16 player thread, video follows playPts
      cAudio audio (2, audioFrames->getSampleRate(), 40000, false);

      cAudioFrame* frame;
      while (!mExit) {
        float* playSamples = silence.data();
          {
          // scoped song mutex
          shared_lock<shared_mutex> lock (audioFrames->getSharedMutex());
          frame = audioFrames->findPlayFrame();
          bool gotSamples = audioFrames->getPlaying() && frame && frame->getSamples();
          if (gotSamples) {
            memcpy (samples.data(), frame->getSamples(), audioFrames->getSamplesPerFrame() * 8);
            playSamples = samples.data();
            }
          }
        audio.play (2, playSamples, audioFrames->getSamplesPerFrame(), 1.f);

        if (frame && audioFrames->getPlaying())
          audioFrames->nextPlayFrame (true);

        if (!streaming && audioFrames->getPlayFinished())
          break;
        }
      //}}}
      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });

    // raise to max prioritu
    sched_param sch_params;
    sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
    pthread_setschedparam (playerThread.native_handle(), SCHED_RR, &sch_params);
    playerThread.detach();
    }
#endif

void cAudioPlayer::wait() {
  while (mRunning)
    this_thread::sleep_for (100ms);
  }
//}}}

// cAudioDecoder
//{{{
cAudioDecoder::cAudioDecoder (const std::string name) : cDecoder(name) {

  eAudioFrameType frameType = eAudioFrameType::eAacLatm;

  switch (frameType) {
    case eAudioFrameType::eMp3:
      cLog::log (LOGINFO, "createAudioDecoder ffmpeg mp3");
      mAudioDecoder = new cFFmpegAudioDecoder (frameType);
      break;

    case eAudioFrameType::eAacAdts:
      cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacAdts");
      mAudioDecoder =  new cFFmpegAudioDecoder (frameType);
      break;

    case eAudioFrameType::eAacLatm:
      cLog::log (LOGINFO, "createAudioDecoder ffmpeg aacLatm");
      mAudioDecoder =  new cFFmpegAudioDecoder (frameType);
      break;

    default:
      cLog::log (LOGERROR, "createAudioDecoder frameType:%d", frameType);
      break;
    }

  mAudioFrames = new cAudioFrames (frameType, 2, 48000, 1024, 100);
  }
//}}}
//{{{
cAudioDecoder::~cAudioDecoder() {

  delete mAudioDecoder;
  delete mAudioFrames;
  }
//}}}

//{{{
bool cAudioDecoder::decode (uint8_t* buf, int bufSize, int64_t pts) {

  log ("pes", fmt::format ("pts:{} size: {}", getFullPtsString (pts), bufSize));
  //logValue (pts, (float)bufSize);

  uint8_t* framePes = buf;
  uint8_t* framePesEnd = buf + bufSize;
  int64_t framePts = pts;

  int frameSize;
  while (parseFrame (framePes, framePesEnd, frameSize)) {
    // decode a single frame from pes
    float* samples = mAudioDecoder->decodeFrame (framePes, frameSize, pts);
    if (samples) {
      cAudioFrame& audioFrame = mAudioFrames->addFrame (pts, samples);

      logValue (framePts, audioFrame.getPowerValues()[0]);

      framePts += (mAudioDecoder->getNumSamplesPerFrame() * 90) / 48;
      }

    // point to next frame in pes
    framePes += frameSize;
    }

  return false;
  }
//}}}

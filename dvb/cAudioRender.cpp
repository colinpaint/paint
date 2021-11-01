// cAudioRender.cpp - simple decoder player
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

#include "cAudioRender.h"

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
constexpr uint32_t kAudioPoolSize = 400;

enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm } ;
namespace {
  //{{{
  uint8_t* audioParseFrame (uint8_t* framePtr, uint8_t* frameLast,
                       eAudioFrameType& frameType, int& numChannels, int& sampleRate, int& frameSize) {
  // simple mp3 / aacAdts / aacLatm / wav / id3Tag frame parser

    frameType = eAudioFrameType::eUnknown;
    numChannels = 0;
    sampleRate = 0;
    frameSize = 0;

    while (framePtr + 10 < frameLast) {
      // got enough for largest header start - id3 tagSize
      if ((framePtr[0] == 0x56) && ((framePtr[1] & 0xE0) == 0xE0)) {
        //{{{  aacLatm syncWord (0x02b7 << 5) found
        frameType = eAudioFrameType::eAacLatm;

        // guess sampleRate and numChannels
        sampleRate = 48000;
        numChannels = 2;

        frameSize = 3 + (((framePtr[1] & 0x1F) << 8) | framePtr[2]);

        // check for enough bytes for frame body
        return (framePtr + frameSize <= frameLast) ? framePtr : nullptr;
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
          // M 13  frame length, this value must include 7 or 9 bytes of header length: frameSize = (ProtectionAbsent == 1 ? 7 : 9) + size(AACFrame)
          // O 11  buffer fullness
          // P  2  Number of AAC frames (RDBs) in ADTS frame minus 1, for maximum compatibility always use 1 AAC frame per ADTS frame
          // Q 16  CRC if protection absent is 0

          frameType = eAudioFrameType::eAacAdts;

          const int sampleRates[16] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0,0,0};
          sampleRate = sampleRates [(framePtr[2] & 0x3c) >> 2];

          // guess numChannels
          numChannels = 2;

          // return aacFrame & size
          frameSize = ((framePtr[3] & 0x3) << 11) | (framePtr[4] << 3) | (framePtr[5] >> 5);

          // check for enough bytes for frame body
          return (framePtr + frameSize <= frameLast) ? framePtr : nullptr;
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

          frameSize = (bitrate * scale) / sampleRate;
          if (pad)
            frameSize++;

          //uint8_t priv = (framePtr[2] & 0x01);
          //uint8_t mode = (framePtr[3] & 0xc0) >> 6;
          numChannels = 2;  // guess
          //uint8_t modex = (framePtr[3] & 0x30) >> 4;
          //uint8_t copyright = (framePtr[3] & 0x08) >> 3;
          //uint8_t original = (framePtr[3] & 0x04) >> 2;
          //uint8_t emphasis = (framePtr[3] & 0x03);

          // check for enough bytes for frame body
          return (framePtr + frameSize <= frameLast) ? framePtr : nullptr;
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
            frameSize = int(frameLast - framePtr);

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
        frameSize = 10 + tagSize;

        // check for enough bytes for frame body
        return (framePtr + frameSize <= frameLast) ? framePtr : nullptr;
        }
        //}}}
      else
        framePtr++;
      }

    return nullptr;
    }
  //}}}
  }

//{{{
class cAudioFrame {
public:
  //{{{
  cAudioFrame (size_t numChannels, size_t samplesPerFrame, int64_t pts, float* samples)
      : mNumChannels(numChannels), mSamplesPerFrame(samplesPerFrame), mPts(pts), mSamples(samples) {

    for (size_t channel = 0; channel < mNumChannels; channel++) {
      // init
      mPeakValues[channel] = 0.f;
      mPowerValues[channel] = 0.f;
      }

    float* samplePtr = samples;
    for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
      // acumulate
      for (size_t channel = 0; channel < mNumChannels; channel++) {
        auto value = *samplePtr++;
        mPeakValues[channel] = max (abs(mPeakValues[channel]), value);
        mPowerValues[channel] += value * value;
        }
      }

    for (size_t channel = 0; channel < mNumChannels; channel++)
      mPowerValues[channel] = sqrtf (mPowerValues[channel] / mSamplesPerFrame);
    }
  //}}}
  //{{{
  ~cAudioFrame() {
    free (mSamples);
    }
  //}}}

  // gets
  int64_t getPts() const { return mPts; }
  float* getSamples() const { return mSamples; }
  std::array<float,6>& getPeakValues() { return mPeakValues; }
  std::array<float,6>& getPowerValues() { return mPowerValues; }

private:
  const size_t mNumChannels = 0;
  const size_t mSamplesPerFrame = 0;
  const int64_t mPts = 0;
  float* mSamples = nullptr;

  std::array <float, 6> mPeakValues = {0.f};
  std::array <float, 6> mPowerValues = {0.f};
  };
//}}}
//{{{
class cAudioPool {
public:
  //{{{
  cAudioPool (eAudioFrameType frameType, int numChannels,
                int sampleRate, int samplesPerFrame, size_t maxMapSize)
    : mFrameType(frameType), mNumChannels(numChannels),
      mSampleRate(sampleRate), mSamplesPerFrame(samplesPerFrame),
      mMaxMapSize(maxMapSize) {}
  //}}}
   //{{{
   ~cAudioPool() {

     unique_lock<shared_mutex> lock (mSharedMutex);

     for (auto& frame : mFrameMap)
       delete (frame.second);
     mFrameMap.clear();
     }
   //}}}

  // gets
  std::shared_mutex& getSharedMutex() { return mSharedMutex; }

  eAudioFrameType getFrameType() const { return mFrameType; }
  uint32_t getNumChannels() const { return mNumChannels; }
  uint32_t getSampleRate() const { return mSampleRate; }
  uint32_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  int64_t getFramePtsDuration() const { return (mSamplesPerFrame * 90000) / mSampleRate; }

  bool getPlaying() const { return mPlaying; }
  int64_t getPlayPts() const { return mPlayPts; }

  // find
  cAudioFrame* findPlayFrame() const { return findFrame (mPlayPts); }

  // play
  void togglePlaying() { mPlaying = !mPlaying; }
  void setPlayPts (int64_t pts) { mPlayPts = pts; }
  void nextPlayFrame() { setPlayPts (mPlayPts + getFramePtsDuration()); }

  // add
  cAudioFrame& addFrame (int64_t pts, float* samples);

private:
  //{{{
  cAudioFrame* findFrame (int64_t pts) const {
    auto it = mFrameMap.find (pts);
    return (it == mFrameMap.end()) ? nullptr : it->second;
    }
  //}}}

  const eAudioFrameType mFrameType;
  const int mNumChannels;
  const int mSampleRate = 0;
  const int mSamplesPerFrame = 0;
  const size_t mMaxMapSize;

  std::shared_mutex mSharedMutex;
  std::map <int64_t, cAudioFrame*> mFrameMap;

  bool mPlaying = false;
  int64_t mPlayPts = 0;
  };

//{{{
cAudioFrame& cAudioPool::addFrame (int64_t pts, float* samples) {

  // create new frame
  cAudioFrame* frame = new cAudioFrame (mNumChannels, mSamplesPerFrame, pts, samples);

    { // locked
    unique_lock<shared_mutex> lock (mSharedMutex);

    if (mFrameMap.size() > mMaxMapSize) // too many, remeove earliest
      mFrameMap.erase (mFrameMap.begin());

    mFrameMap.emplace (pts, frame);
    }

  return *frame;
  }
//}}}
//}}}
//{{{
class cAudioDecoder {
public:
  cAudioDecoder (eAudioFrameType frameType);
  ~cAudioDecoder();

  size_t getNumChannels() { return mChannels; }
  size_t getSampleRate() { return mSampleRate; }
  size_t getNumSamplesPerFrame() { return mSamplesPerFrame; }

  float* decodeFrame (const uint8_t* frame, uint32_t frameSize, int64_t pts);

private:
  size_t mChannels = 0;
  size_t mSampleRate = 0;
  size_t mSamplesPerFrame = 0;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;

  int64_t mLastPts = -1;
  };

// cAudioDecoder members
//{{{
cAudioDecoder::cAudioDecoder (eAudioFrameType frameType) {

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
      cLog::log (LOGERROR, "unknown cAacDecoder frameType %d", frameType);
      return;
    }

  mAvParser = av_parser_init (streamType);
  mAvCodec = avcodec_find_decoder (streamType);
  mAvContext = avcodec_alloc_context3 (mAvCodec);
  avcodec_open2 (mAvContext, mAvCodec, NULL);
  }
//}}}
//{{{
cAudioDecoder::~cAudioDecoder() {

  if (mAvContext)
    avcodec_close (mAvContext);
  if (mAvParser)
    av_parser_close (mAvParser);
  }
//}}}

//{{{
float* cAudioDecoder::decodeFrame (const uint8_t* frame, uint32_t frameSize, int64_t pts) {

  float* outBuffer = nullptr;

  AVPacket avPacket;
  av_init_packet (&avPacket);
  auto avFrame = av_frame_alloc();

  while (frameSize) {
    int bytesUsed = av_parser_parse2 (mAvParser, mAvContext,
                                      &avPacket.data, &avPacket.size,
                                      frame, frameSize, pts, AV_NOPTS_VALUE, 0);
    frame += bytesUsed;
    frameSize -= bytesUsed;

    avPacket.pts = pts;
    if (avPacket.size) {
      int ret = avcodec_send_packet (mAvContext, &avPacket);
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
              if (avFrame->channels == 6) {
                // 5.1 mix down
                float* srcPtr2 = (float*)avFrame->data[2];
                float* srcPtr3 = (float*)avFrame->data[3];
                float* srcPtr4 = (float*)avFrame->data[4];
                float* srcPtr5 = (float*)avFrame->data[5];
                for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++ + *srcPtr2++ + *srcPtr4 + *srcPtr5; // left loud
                  *dstPtr++ = *srcPtr1++ + *srcPtr3++ + *srcPtr4++ + *srcPtr5++; // right loud
                  }
                }
              else {
                // stereo
                for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr++ = *srcPtr0++;
                  *dstPtr++ = *srcPtr1++;
                  }
                }

              break;
              }
            //}}}
            //{{{
            case AV_SAMPLE_FMT_S16P: { // 16bit signed planar, copy scale and copy to interleaved

              mChannels =  avFrame->channels;
              mSampleRate = avFrame->sample_rate;
              mSamplesPerFrame = avFrame->nb_samples;

              outBuffer = (float*)malloc (avFrame->channels * avFrame->nb_samples * sizeof(float));

              for (int channel = 0; channel < avFrame->channels; channel++) {
                auto dstPtr = outBuffer + channel;
                auto srcPtr = (short*)avFrame->data[channel];
                for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
                  *dstPtr = *srcPtr++ / (float)0x8000;
                  dstPtr += mChannels;
                  }
                }
              break;
              }
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
  cAudioPlayer (cAudioPool* audioFrames);
  ~cAudioPlayer() {}

  void exit() { mExit = true; }
  void wait();

private:
  bool mRunning = true;
  bool mExit = false;
  };

#ifdef _WIN32
  //{{{
  cAudioPlayer::cAudioPlayer (cAudioPool* audioFrames) {
    thread playerThread = thread ([=]() {
      // lambda
      cLog::setThreadName ("play");
      SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
      array <float,2048*2> silence = { 0.f };
      array <float,2048*2> samples = { 0.f };

      audioFrames->togglePlaying();

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
                memcpy (samples.data(),
                        frame->getSamples(),
                        audioFrames->getSamplesPerFrame() * audioFrames->getNumChannels() * sizeof(float));
              srcSamples = samples.data();
              }
            else
              srcSamples = silence.data();
            numSrcSamples = audioFrames->getSamplesPerFrame();

            if (frame && audioFrames->getPlaying())
              audioFrames->nextPlayFrame();
            });
          }

        device->stop();
        }

      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });

    playerThread.detach();
    }
  //}}}
#else
  //{{{
  cAudioPlayer::cAudioPlayer (cAudioPool* audioFrames) {
    thread playerThread = thread ([=, this]() {
      // lambda
      cLog::setThreadName ("play");
      array <float,2048*2> silence = { 0.f };
      array <float,2048*2> samples = { 0.f };

      audioFrames->togglePlaying();

      // audio16 player thread, video follows playPts
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
          audioFrames->nextPlayFrame();
        }

      mRunning = false;
      cLog::log (LOGINFO, "exit");
      });

    // raise to max prioritu
    sched_param sch_params;
    sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
    pthread_setschedparam (playerThread.native_handle(), SCHED_RR, &sch_params);
    playerThread.detach();
    }
  //}}}
#endif

//{{{
void cAudioPlayer::wait() {
  while (mRunning)
    this_thread::sleep_for (100ms);
  }
//}}}
//}}}

// cAudio
//{{{
cAudioRender::cAudioRender (const std::string name) : cRender(name) {

  eAudioFrameType frameType = eAudioFrameType::eAacLatm;

  switch (frameType) {
    //{{{
    case eAudioFrameType::eMp3:
      cLog::log (LOGINFO, "createAudioDecoder mp3");
      mAudioDecoder = new cAudioDecoder (frameType);
      break;
    //}}}
    //{{{
    case eAudioFrameType::eAacAdts:
      cLog::log (LOGINFO, "createAudioDecoder aacAdts");
      mAudioDecoder =  new cAudioDecoder (frameType);
      break;
    //}}}
    //{{{
    case eAudioFrameType::eAacLatm:
      cLog::log (LOGINFO, "createAudioDecoder aacLatm");
      mAudioDecoder =  new cAudioDecoder (frameType);
      break;
    //}}}
    //{{{
    default:
      cLog::log (LOGERROR, "createAudioDecoder frameType:%d", frameType);
      break;
    //}}}
    }

  mAudioPool = new cAudioPool (frameType, 2, 48000, 1024, kAudioPoolSize);
  }
//}}}
//{{{
cAudioRender::~cAudioRender() {

  if (mAudioPlayer) {
    mAudioPlayer->exit();
    mAudioPlayer->wait();
    }

  delete mAudioDecoder;
  delete mAudioPool;
  delete mAudioPlayer;
  }
//}}}

//{{{
int64_t cAudioRender::getPlayPts() const {
  return mAudioPool->getPlayPts();
  }
//}}}

//{{{
void cAudioRender::processPes (uint8_t* pes, uint32_t pesSize, int64_t pts) {

  log ("pes", fmt::format ("pts:{} size: {}", getFullPtsString (pts), pesSize));
  //logValue (pts, (float)bufSize);

  uint8_t* frame = pes;
  uint8_t* pesEnd = pes + pesSize;
  int64_t framePts = pts;

  // parse pesFrame, pes may have more than one frame
  eAudioFrameType frameType = eAudioFrameType::eUnknown;
  int numChannels = 0;
  int sampleRate = 0;
  int frameSize = 0;
  while (audioParseFrame (frame, pesEnd, frameType, numChannels, sampleRate, frameSize)) {
    // decode single frame from pes
    float* samples = mAudioDecoder->decodeFrame (frame, frameSize, framePts);
    if (samples) {
      cAudioFrame& audioFrame = mAudioPool->addFrame (framePts, samples);
      logValue (framePts, audioFrame.getPowerValues()[0]);

      if (!mAudioPlayer) {
        /// start player
        mAudioPool->setPlayPts (framePts);
        mAudioPlayer = new cAudioPlayer (mAudioPool);
        }

      // inc pts for nextFrame
      framePts += mAudioPool->getFramePtsDuration();
      }

    // point to nextFrame
    frame += frameSize;
    }
  }
//}}}

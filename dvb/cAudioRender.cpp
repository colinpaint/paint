// cAudioRender.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cAudioRender.h"

#ifdef _WIN32
  #include "../audio/audioWASAPI.h"
  #include "../audio/cWinAudio16.h"
#else
  #include "../audio/cLinuxAudio.h"
#endif

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>
#include <shared_mutex>
#include <thread>
#include <functional>

#include "../imgui/imgui.h"
#include "../imgui/myImgui.h"

#include "cFrame.h"

#include "../utils/date.h"
#include "../utils/cLog.h"
#include "../utils/utils.h"

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

using namespace std;
//}}}

constexpr uint32_t kAudioPoolSize = 50;
//{{{
class cAudioFrame : public cFrame {
public:
  //{{{
  cAudioFrame (int64_t pts, size_t numChannels, size_t samplesPerFrame, uint32_t sampleRate, float* samples)
     : cFrame (pts, samplesPerFrame * 90000 / sampleRate),
       mNumChannels(numChannels), mSamplesPerFrame(samplesPerFrame), mSampleRate(sampleRate), mSamples(samples) {

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

  //{{{
  static uint8_t* audioParseFrame (uint8_t* framePtr, uint8_t* frameLast,
                            eAudioFrameType& frameType, size_t& numChannels,
                            uint32_t& sampleRate, uint32_t& frameSize) {
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

          const uint32_t sampleRates[16] = { 96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0,0,0};
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

          const uint32_t sampleRates[4] = { 44100, 48000, 32000, 0};
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

  // gets
  size_t getNumChannels() const { return mNumChannels; }
  size_t getSamplesPerFrame () const { return mSamplesPerFrame; }
  uint32_t getSampleRate() const { return mSampleRate; }

  int64_t getPts() const { return mPts; }
  int64_t getPtsDuration() const { return mPtsDuration; }

  float* getSamples() const { return mSamples; }

  std::array<float,6>& getPeakValues() { return mPeakValues; }
  std::array<float,6>& getPowerValues() { return mPowerValues; }

private:
  const size_t mNumChannels;
  const size_t mSamplesPerFrame;
  const uint32_t mSampleRate;

  float* mSamples = nullptr;

  std::array <float, 6> mPeakValues = {0.f};
  std::array <float, 6> mPowerValues = {0.f};
  };
//}}}
//{{{
class cAudioDecoder {
public:
  //{{{
  cAudioDecoder (eAudioFrameType frameType) {

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
  ~cAudioDecoder() {

    if (mAvContext)
      avcodec_close (mAvContext);
    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  int64_t decodeFrame (uint8_t* pes, uint32_t pesSize, int64_t pts,
                       function<void (cAudioFrame* audioFrame)> addFrameCallback) {

    uint8_t* frame = pes;
    uint8_t* pesEnd = pes + pesSize;

    // parse pesFrame, pes may have more than one frame
    eAudioFrameType frameType = eAudioFrameType::eUnknown;
    size_t numChannels = 0;
    size_t samplesPerFrame = 0;
    uint32_t sampleRate = 0;
    uint32_t frameSize = 0;
    while (cAudioFrame::audioParseFrame (frame, pesEnd, frameType, numChannels, sampleRate, frameSize)) {
      AVPacket avPacket;
      av_init_packet (&avPacket);

      auto avFrame = av_frame_alloc();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext,
                                        &avPacket.data, &avPacket.size,
                                        frame, frameSize, pts, AV_NOPTS_VALUE, 0);
      avPacket.pts = pts;
      if (avPacket.size) {
        int ret = avcodec_send_packet (mAvContext, &avPacket);
        while (ret >= 0) {
          ret = avcodec_receive_frame (mAvContext, avFrame);
          if ((ret == AVERROR(EAGAIN)) || (ret == AVERROR_EOF) || (ret < 0))
            break;
          if (avFrame->nb_samples > 0) {
            samplesPerFrame = avFrame->nb_samples;
            sampleRate = avFrame->sample_rate;
            float* samples = nullptr;
            switch (mAvContext->sample_fmt) {
              //{{{
              case AV_SAMPLE_FMT_FLTP: { // 32bit float planar, copy to interleaved, mix down 5.1

                size_t srcNumChannels = avFrame->channels;

                numChannels = 2;
                samples = (float*)malloc (numChannels * samplesPerFrame * sizeof(float));
                float* dstPtr = samples;

                float* srcPtr0 = (float*)avFrame->data[0];
                float* srcPtr1 = (float*)avFrame->data[1];
                if (srcNumChannels == 6) {
                  // 5.1 mix down
                  float* srcPtr2 = (float*)avFrame->data[2];
                  float* srcPtr3 = (float*)avFrame->data[3];
                  float* srcPtr4 = (float*)avFrame->data[4];
                  float* srcPtr5 = (float*)avFrame->data[5];
                  for (size_t sample = 0; sample < samplesPerFrame; sample++) {
                    *dstPtr++ = *srcPtr0++ + *srcPtr2++ + *srcPtr4 + *srcPtr5; // left loud
                    *dstPtr++ = *srcPtr1++ + *srcPtr3++ + *srcPtr4++ + *srcPtr5++; // right loud
                    }
                  }
                else {
                  // stereo
                  for (size_t sample = 0; sample < samplesPerFrame; sample++) {
                    *dstPtr++ = *srcPtr0++;
                    *dstPtr++ = *srcPtr1++;
                    }
                  }

                break;
                }
              //}}}
              //{{{
              case AV_SAMPLE_FMT_S16P: { // 16bit signed planar, copy scale and copy to interleaved

                numChannels = avFrame->channels;
                samples = (float*)malloc (numChannels * samplesPerFrame * sizeof(float));

                for (size_t channel = 0; channel < numChannels; channel++) {
                  auto dstPtr = samples + channel;
                  auto srcPtr = (short*)avFrame->data[channel];
                  for (size_t sample = 0; sample < samplesPerFrame; sample++) {
                    *dstPtr = *srcPtr++ / (float)0x8000;
                    dstPtr += numChannels;
                    }
                  }
                break;
                }
              //}}}
              default:;
              }

            cAudioFrame* audioFrame = new cAudioFrame (pts, numChannels, samplesPerFrame, sampleRate, samples);
            addFrameCallback (audioFrame);
            av_frame_unref (avFrame);
            pts += audioFrame->getPtsDuration();
            }
          }
        }
      av_frame_free (&avFrame);

      frame += bytesUsed;
      }

    return pts;
    }

private:
  size_t mChannels = 0;
  size_t mSampleRate = 0;
  size_t mSamplesPerFrame = 0;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodec* mAvCodec = nullptr;
  AVCodecContext* mAvContext = nullptr;

  int64_t mLastPts = -1;
  };
//}}}
//{{{
class cAudioPlayer {
public:
  #ifdef _WIN32
    //{{{
    cAudioPlayer (cAudioRender* audio, int64_t pts) {

      mPts = pts;
      mThread = thread ([=]() {
        cLog::setThreadName ("play");
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        array <float,2048*2> silence = { 0.f };
        array <float,2048*2> samples = { 0.f };

        auto audioDevice = getDefaultAudioOutputDevice();
        if (audioDevice) {
          cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", audio->getSampleRate());
          audioDevice->setSampleRate (audio->getSampleRate());
          audioDevice->start();

          cAudioFrame* audioFrame;
          while (!mExit) {
            audioDevice->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
              // loadSrcSamples callback lambda
              shared_lock<shared_mutex> lock (audio->getSharedMutex());
              audioFrame = audio->findPlayFrame();
              if (mPlaying && audioFrame && audioFrame->getSamples()) {
                if (audioFrame->getNumChannels() == 1) {
                  // convert mono to 2 channels
                  float* src = audioFrame->getSamples();
                  float* dst = samples.data();
                  for (size_t i = 0; i < audioFrame->getSamplesPerFrame(); i++) {
                    *dst++ = *src;
                    *dst++ = *src++;
                    }
                  }
                else
                  memcpy (samples.data(), audioFrame->getSamples(),
                          audioFrame->getSamplesPerFrame() * audioFrame->getNumChannels() * sizeof(float));
                srcSamples = samples.data();
                }
              else
                srcSamples = silence.data();
              numSrcSamples = audioFrame ? audioFrame->getSamplesPerFrame() : 1024;

              if (mPlaying && audioFrame)
                mPts += audioFrame->getPtsDuration();
              });
            }

          audioDevice->stop();
          }

        mRunning = false;
        cLog::log (LOGINFO, "exit");
        });

      mThread.detach();
      }
    //}}}
  #else
    //{{{
    cAudioPlayer (cAudioRender* audio int64_t pts) {

      mPts = pts;
      mThread = thread ([=, this]() {
        // lambda
        cLog::setThreadName ("play");
        array <float,2048*2> silence = { 0.f };
        array <float,2048*2> samples = { 0.f };

        audio->togglePlaying();

        // audio16 player thread, video follows playPts
        cAudio audioDevice (2, audio->getSampleRate(), 40000, false);
        cAudioFrame* audioFrame;
        while (!mExit) {
          float* playSamples = silence.data();
            {
            // scoped song mutex
            shared_lock<shared_mutex> lock (audio->getSharedMutex());
            audioFrame = audio->findPlayFrame();
            if (mPlaying && audioFrame && audioFrame->getSamples()) {
              memcpy (samples.data(), audioFrame->getSamples(), audioFrame->getSamplesPerFrame() * 8);
              playSamples = samples.data();
              }
            }
          audioDevice.play (2, playSamples, audioFrame ? audioFrame->getSamplesPerFrame() : 1024, 1.f);

          if (mPlaying && audioFrame)
            mPts += audioFrame->getPtsDuration());
          }

        mRunning = false;
        cLog::log (LOGINFO, "exit");
        });

      // raise to max prioritu
      sched_param sch_params;
      sch_params.sched_priority = sched_get_priority_max (SCHED_RR);
      pthread_setschedparam (mThread.native_handle(), SCHED_RR, &sch_params);
      mThread.detach();
      }
    //}}}
  #endif

  ~cAudioPlayer() = default;

  bool getPlaying() const { return mPlaying; }
  int64_t getPts() const { return mPts; }

  void togglePlaying() { mPlaying = !mPlaying; }
  void setPts (int64_t pts) { mPts = pts; }

  void exit() { mExit = true; }
  //{{{
  void wait() {
    while (mRunning)
      this_thread::sleep_for (100ms);
    }
  //}}}

private:
  bool mPlaying = true;
  bool mRunning = true;
  bool mExit = false;

  int64_t mPts = 0;
  thread mThread;
  };
//}}}

// cAudioRender
//{{{
cAudioRender::cAudioRender (const std::string name)
    : cRender(name) {

  mFrameType = eAudioFrameType::eAacLatm;

  mNumChannels = 2;
  mSampleRate = 48000;
  mSamplesPerFrame = 1024;
  mMaxMapSize = kAudioPoolSize;

  switch (mFrameType) {
    //{{{
    case eAudioFrameType::eMp3:
      cLog::log (LOGINFO, "createAudioDecoder mp3");
      mDecoder = new cAudioDecoder (mFrameType);
      break;
    //}}}
    //{{{
    case eAudioFrameType::eAacAdts:
      cLog::log (LOGINFO, "createAudioDecoder aacAdts");
      mDecoder =  new cAudioDecoder (mFrameType);
      break;
    //}}}
    //{{{
    case eAudioFrameType::eAacLatm:
      cLog::log (LOGINFO, "createAudioDecoder aacLatm");
      mDecoder =  new cAudioDecoder (mFrameType);
      break;
    //}}}
    //{{{
    default:
      cLog::log (LOGERROR, "createAudioDecoder frameType:%d", mFrameType);
      break;
    //}}}
    }
  }
//}}}
//{{{
cAudioRender::~cAudioRender() {

  if (mPlayer) {
    mPlayer->exit();
    mPlayer->wait();
    }
  delete mPlayer;

  delete mDecoder;

  unique_lock<shared_mutex> lock (mSharedMutex);
  for (auto& frame : mFrames)
    delete (frame.second);
  mFrames.clear();
  }
//}}}

//{{{
bool cAudioRender::getPlaying() const {
  return mPlayer ? mPlayer->getPlaying() : false;
  }
//}}}
//{{{
int64_t cAudioRender::getPlayPts() const {
  return mPlayer ? mPlayer->getPts() : 0;
  }
//}}}
//{{{
string cAudioRender::getInfoString() const {
  return fmt::format ("{}", mFrames.size());
  }
//}}}

//{{{
void cAudioRender::togglePlaying() {
  if (mPlayer)
    mPlayer->togglePlaying();
  }
//}}}
//{{{
void cAudioRender::setPlayPts (int64_t pts) {
  if (mPlayer)
    mPlayer->setPts (pts);
  }
//}}}

//{{{
cAudioFrame* cAudioRender::findFrame (int64_t pts) const {

  auto it = mFrames.find (pts);
  return (it == mFrames.end()) ? nullptr : it->second;
  }
//}}}
//{{{
cAudioFrame* cAudioRender::findPlayFrame() const {
  return findFrame (getPlayPts());
  }
//}}}

//{{{
void cAudioRender::processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts) {

  (void)dts;
  log ("pes", fmt::format ("pts:{} size: {}", getFullPtsString (pts), pesSize));
  //logValue (pts, (float)bufSize);

  mDecoder->decodeFrame (pes, pesSize, pts,
    [&](cAudioFrame* audioFrame) noexcept {
      mNumChannels = audioFrame->getNumChannels();
      mSampleRate = audioFrame->getSampleRate();
      mSamplesPerFrame = audioFrame->getSamplesPerFrame();

      logValue (audioFrame->getPts(), audioFrame->getPowerValues()[0]);
      addFrame (audioFrame);

      if (!mPlayer)
        mPlayer = new cAudioPlayer (this, audioFrame->getPts());
      }
    );

  }
//}}}
//{{{
void cAudioRender::addFrame (cAudioFrame* frame) {

    { // locked emplace and erase
    unique_lock<shared_mutex> lock (mSharedMutex);
    mFrames.emplace (frame->getPts(), frame);

    if (mFrames.size() >= mMaxMapSize) {
      auto it = mFrames.begin();
      while ((*it).first < getPlayPts()) {
        // remove frames before mPlayPts
        auto frameToRemove = (*it).second;
        it = mFrames.erase (it);

        // !!! should give back to free list !!!
        delete frameToRemove;
        }
      }
    }

  if (mFrames.size() >= mMaxMapSize)
    this_thread::sleep_for (20ms);
  }
//}}}

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
#include <functional>
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
constexpr bool kQueued = false;
constexpr uint32_t kAudioPoolSize = 50;

//{{{
class cAudioFrame : public cFrame {
public:
  //{{{
  cAudioFrame (int64_t pts, size_t numChannels, size_t samplesPerFrame, uint32_t sampleRate, float* samples)
     : cFrame (pts, sampleRate ? (samplesPerFrame * 90000 / sampleRate) : 48000),
       mNumChannels(numChannels),
       mSamplesPerFrame(samplesPerFrame), mSampleRate(sampleRate), mSamples(samples) {

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
class cFFmpegAudioDecoder : public cDecoder {
public:
  //{{{
  cFFmpegAudioDecoder (uint8_t streamType) : cDecoder(),
    mAvCodec (avcodec_find_decoder ((streamType == 17) ? AV_CODEC_ID_AAC_LATM : AV_CODEC_ID_MP3)) {

    cLog::log (LOGINFO, fmt::format ("cFFmpegAudioDecoder stream:{}", streamType));

    // aacAdts AV_CODEC_ID_AAC;
    mAvParser = av_parser_init ((streamType == 17) ? AV_CODEC_ID_AAC_LATM : AV_CODEC_ID_MP3);
    mAvContext = avcodec_alloc_context3 (mAvCodec);
    avcodec_open2 (mAvContext, mAvCodec, NULL);
    }
  //}}}
  //{{{
  virtual ~cFFmpegAudioDecoder() {

    if (mAvContext)
      avcodec_close (mAvContext);
    if (mAvParser)
      av_parser_close (mAvParser);
    }
  //}}}

  virtual string getName() const final { return "ffmpeg"; }
  //{{{
  virtual int64_t decode (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts,
                          function<void (cFrame* frame)> addFrameCallback) final  {
    (void)dts;
    int64_t interpolatedPts = pts;

    // parse pesFrame, pes may have more than one frame
    size_t numChannels = 0;
    size_t samplesPerFrame = 0;
    uint32_t sampleRate = 48000;

    uint8_t* pesPtr = pes;
    while (pesSize) {
      AVPacket* avPacket = av_packet_alloc();
      AVFrame* avFrame = av_frame_alloc();
      int bytesUsed = av_parser_parse2 (mAvParser, mAvContext, &avPacket->data, &avPacket->size,
                                        pesPtr, (int)pesSize, 0, 0, AV_NOPTS_VALUE);
      pesPtr += bytesUsed;
      pesSize -= bytesUsed;
      if (avPacket->size) {
        int ret = avcodec_send_packet (mAvContext, avPacket);
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

            cAudioFrame* audioFrame = new cAudioFrame (interpolatedPts, numChannels,
                                                       samplesPerFrame, sampleRate, samples);
            addFrameCallback (audioFrame);
            interpolatedPts += audioFrame->getPtsDuration();
            }
          }
        }
      av_frame_free (&avFrame);
      av_packet_free (&avPacket);
      }

    return interpolatedPts;
    }
  //}}}

private:
  const AVCodec* mAvCodec = nullptr;

  AVCodecParserContext* mAvParser = nullptr;
  AVCodecContext* mAvContext = nullptr;

  size_t mChannels = 0;
  size_t mSampleRate = 0;
  size_t mSamplesPerFrame = 0;
  };
//}}}
//{{{
class cAudioPlayer {
public:
  #ifdef _WIN32
    //{{{
    cAudioPlayer (cAudioRender* audioRender, int64_t pts) {

      mPts = pts;

      mThread = thread ([=]() {
        cLog::setThreadName ("play");
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

        auto audioDevice = getDefaultAudioOutputDevice();
        if (audioDevice) {
          cLog::log (LOGINFO, "startPlayer WASPI device:%dhz", audioRender->getSampleRate());
          audioDevice->setSampleRate (audioRender->getSampleRate());
          audioDevice->start();

          array <float,2048*2> samples = { 0.f };
          array <float,2048*2> silence = { 0.f };
          cAudioFrame* audioFrame;
          while (!mExit) {
            audioDevice->process ([&](float*& srcSamples, int& numSrcSamples) mutable noexcept {
              // loadSrcSamples callback lambda
              shared_lock<shared_mutex> lock (audioRender->getSharedMutex());
              audioFrame = audioRender->findPlayFrame();
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
              numSrcSamples = (int)(audioFrame ? audioFrame->getSamplesPerFrame() : audioRender->getSamplesPerFrame());

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
    cAudioPlayer (cAudioRender* audioRender, int64_t pts) {

      mPts = pts;

      mThread = thread ([=]() { //,this
        // lambda
        cLog::setThreadName ("play");

        // audio16 player thread
        cAudio audioDevice (2, audioRender->getSampleRate(), 40000, false);

        array <float,2048*2> samples = { 0.f };
        array <float,2048*2> silence = { 0.f };
        cAudioFrame* audioFrame;
        while (!mExit) {
          float* srcSamples = silence.data();
            {
            // scoped song mutex
            shared_lock<shared_mutex> lock (audioRender->getSharedMutex());
            audioFrame = audioRender->findPlayFrame();
            if (mPlaying && audioFrame && audioFrame->getSamples()) {
              memcpy (samples.data(), audioFrame->getSamples(), audioFrame->getSamplesPerFrame() * 8);
              srcSamples = samples.data();
              }
            }

          audioDevice.play (
            2, srcSamples, audioFrame ? audioFrame->getSamplesPerFrame() : audioRender->getSamplesPerFrame(), 1.f);

          if (mPlaying && audioFrame)
            mPts += audioFrame->getPtsDuration();
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
cAudioRender::cAudioRender (const string& name, uint8_t streamType, uint16_t decoderMask)
    : cRender(kQueued, name, streamType, decoderMask) {

  mNumChannels = 2;
  mSampleRate = 48000;
  mSamplesPerFrame = 1024;
  mMaxMapSize = kAudioPoolSize;

  mDecoder = new cFFmpegAudioDecoder (streamType);
  }
//}}}
//{{{
cAudioRender::~cAudioRender() {

  if (mPlayer) {
    mPlayer->exit();
    mPlayer->wait();
    }
  delete mPlayer;

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
string cAudioRender::getInfo() const {
  return fmt::format ("{}", mFrames.size());
  }
//}}}
//{{{
void cAudioRender::addFrame (cFrame* frame) {

  cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);

  mNumChannels = audioFrame->getNumChannels();
  mSampleRate = audioFrame->getSampleRate();
  mSamplesPerFrame = audioFrame->getSamplesPerFrame();

  logValue (audioFrame->getPts(), audioFrame->getPowerValues()[0]);

    { // locked emplace and erase
    unique_lock<shared_mutex> lock (mSharedMutex);
    mFrames.emplace (audioFrame->getPts(), audioFrame);

    if (mFrames.size() >= mMaxMapSize) {
      auto it = mFrames.begin();
      while ((*it).first < getPlayPts()) {
        // remove frames before mPlayPts
        auto frameToRemove = (*it).second;
        it = mFrames.erase (it);

        // !!! could give back to free list !!!
        delete frameToRemove;
        }
      }
    }

  if (mFrames.size() >= mMaxMapSize)
    this_thread::sleep_for (20ms);


  if (!mPlayer)
    mPlayer = new cAudioPlayer (this, audioFrame->getPts());
  }
//}}}

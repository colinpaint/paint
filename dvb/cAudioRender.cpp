// cAudioRender.cpp
//{{{  includes
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include "cAudioRender.h"
#include "cAudioFrame.h"

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
constexpr size_t kAudioFrameMapSize = 6;

//{{{
class cFFmpegAudioDecoder : public cDecoder {
public:
  //{{{
  cFFmpegAudioDecoder (uint8_t streamTypeId) : cDecoder(),
    mAvCodec (avcodec_find_decoder ((streamTypeId == 17) ? AV_CODEC_ID_AAC_LATM : AV_CODEC_ID_MP3)) {

    cLog::log (LOGINFO, fmt::format ("cFFmpegAudioDecoder stream:{}", streamTypeId));

    // aacAdts AV_CODEC_ID_AAC;
    mAvParser = av_parser_init ((streamTypeId == 17) ? AV_CODEC_ID_AAC_LATM : AV_CODEC_ID_MP3);
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
                          function<cFrame* ()> getFrameCallback,
                          function<void (cFrame* frame)> addFrameCallback) final  {
    (void)dts;
    int64_t interpolatedPts = pts;

    // parse pesFrame, pes may have more than one frame
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
            cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(getFrameCallback());
            audioFrame->mPts = interpolatedPts;
            audioFrame->mPtsDuration = avFrame->sample_rate ? (avFrame->nb_samples * 90000 / avFrame->sample_rate) : 48000;
            audioFrame->mPesSize = pesSize;
            audioFrame->mSamplesPerFrame = avFrame->nb_samples;
            audioFrame->mSampleRate = avFrame->sample_rate;
            audioFrame->mNumChannels = avFrame->channels;

            float* dst = audioFrame->mSamples;
            switch (mAvContext->sample_fmt) {
              case AV_SAMPLE_FMT_FLTP:  // 32bit float planar, copy to interleaved
                for (size_t sample = 0; sample < avFrame->nb_samples; sample++)
                  for (size_t channel = 0; channel < avFrame->channels; channel++)
                    *dst++ = *(((float*)avFrame->data[channel]) + sample);
                break;
              case AV_SAMPLE_FMT_S16P:  // 16bit signed planar, scale to interleaved
                for (size_t sample = 0; sample < avFrame->nb_samples; sample++)
                  for (size_t channel = 0; channel < avFrame->channels; channel++)
                    *dst++ = (*(((short*)avFrame->data[channel]) + sample)) / (float)0x8000;
                break;
              }

            audioFrame->calcSamples();
            addFrameCallback (audioFrame);
            interpolatedPts += audioFrame->getPtsDuration();
            }
          }
        }

      av_frame_unref (avFrame);
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
              audioFrame = audioRender->getAudioFramePts (mPts);
              if (mPlaying && audioFrame && audioFrame->getSamples()) {
                float* src = audioFrame->getSamples();
                float* dst = samples.data();
                if (audioFrame->getNumChannels() == 1) {
                  // convert mono to 2 channels
                  for (size_t i = 0; i < audioFrame->getSamplesPerFrame(); i++) {
                    *dst++ = *src;
                    *dst++ = *src++;
                    }
                  }
                else if (audioFrame->getNumChannels() == 6) {
                  for (size_t i = 0; i < audioFrame->getSamplesPerFrame(); i++) {
                    *dst++ = src[0] + src[2] + src[4] + src[5]; // left loud
                    *dst++ = src[1] + src[3] + src[4] + src[5]; // right loud
                    src += 6;
                    }
                  }
                else
                  memcpy (dst, src, audioFrame->getSamplesPerFrame() * audioFrame->getNumChannels() * sizeof(float));
                srcSamples = samples.data();
                }
              else
                srcSamples = silence.data();
              numSrcSamples = (int)(audioFrame ? audioFrame->getSamplesPerFrame() : audioRender->getSamplesPerFrame());

              if (mPlaying)
                mPts += audioFrame ? audioFrame->getPtsDuration() : audioRender->getPtsDuration();
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
            audioFrame = audioRender->getAudioFramePts (mPts);
            if (mPlaying && audioFrame && audioFrame->getSamples()) {
              if (audioFrame->getNumChannels() == 6) {
                float* src = audioFrame->getSamples();
                float* dst = samples.data();
                for (size_t i = 0; i < audioFrame->getSamplesPerFrame(); i++) {
                  *dst++ = src[0] + src[2] + src[4] + src[5]; // left loud
                  *dst++ = src[1] + src[3] + src[4] + src[5]; // right loud
                  src += 6;
                  }
                srcSamples = samples.data();
                }
              else {
                memcpy (samples.data(), audioFrame->getSamples(), audioFrame->getSamplesPerFrame() * 8);
                srcSamples = samples.data();
                }
              }
            }

          audioDevice.play (2, srcSamples,
                            audioFrame ? audioFrame->getSamplesPerFrame() : audioRender->getSamplesPerFrame(), 1.f);

          if (mPlaying)
            mPts += audioFrame ? audioFrame->getPtsDuration() : audioRender->getPtsDuration();
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

  bool isPlaying() const { return mPlaying; }
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
cAudioRender::cAudioRender (const string& name, uint8_t streamTypeId, uint16_t decoderMask)
    : cRender(kQueued, name, streamTypeId, decoderMask, kAudioFrameMapSize) {

  mNumChannels = 2;
  mSampleRate = 48000;
  mSamplesPerFrame = 1024;

  mDecoder = new cFFmpegAudioDecoder (streamTypeId);

  setGetFrameCallback ([&]() noexcept { return getFrame(); });
  setAddFrameCallback ([&](cFrame* frame) noexcept { addFrame (frame); });
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
bool cAudioRender::isPlaying() const {
  return mPlayer ? mPlayer->isPlaying() : false;
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
cAudioFrame* cAudioRender::getAudioFramePts (int64_t pts) {
  return dynamic_cast<cAudioFrame*>(getFramePts (pts));
  }
//}}}

// callback
//{{{
cFrame* cAudioRender::getFrame() {

  cFrame* frame = getFreeFrame();
  if (frame)
    return frame;

  return new cAudioFrame();
  }
//}}}
//{{{
void cAudioRender::addFrame (cFrame* frame) {

  cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);

  mNumChannels = audioFrame->getNumChannels();
  mSampleRate = audioFrame->getSampleRate();
  mSamplesPerFrame = audioFrame->getSamplesPerFrame();
  mPtsDuration = audioFrame->getPtsDuration();

  logValue (audioFrame->getPts(), audioFrame->getPowerValues()[0]);

  { // locked emplace
  unique_lock<shared_mutex> lock (mSharedMutex);
  mFrames.emplace (audioFrame->getPts(), audioFrame);
  }

  // start player
  if (!mPlayer)
    mPlayer = new cAudioPlayer (this, audioFrame->getPts());

  // trim before playPts
  trimFramesBeforePts (getPlayPts());

  // throttle on number of queued audio frames
  if (mFrames.size() >= mFrameMapSize)
    this_thread::sleep_for (20ms);
  }
//}}}

// virtual
//{{{
string cAudioRender::getInfo() const {
  return fmt::format ("{}:{}", mFrames.size(), mFreeFrames.size());
  }
//}}}

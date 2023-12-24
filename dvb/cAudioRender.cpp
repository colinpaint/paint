// cAudioRender.cpp
//{{{  includes
#ifdef _WIN32
  #define _CRT_SECURE_NO_WARNINGS
  #define WIN32_LEAN_AND_MEAN
#endif

#include "cAudioRender.h"

#include <cstdint>
#include <string>
#include <array>
#include <algorithm>
#include <functional>
#include <thread>

#include "../common/date.h"
#include "../common/cLog.h"
#include "../common/utils.h"

//{{{  include libav
#ifdef _WIN32
  #pragma warning (push)
  #pragma warning (disable: 4244)
#endif

extern "C" {
  #include "libavcodec/avcodec.h"
  #include "libavformat/avformat.h"
  }

#ifdef _WIN32
  #pragma warning (pop)
#endif
//}}}
#include "../decoders/cAudioParser.h"
#include "../decoders/cFFmpegAudioDecoder.h"
#include "cPlayer.h"

using namespace std;
//}}}
constexpr bool kQueued = true;
constexpr size_t kLiveMaxFrames = 24;
constexpr size_t kFileMaxFrames = 48;
constexpr int64_t kDefaultPtsPerFrame = 1920;

// cAudioRender
//{{{
cAudioRender::cAudioRender (const string& name, uint8_t streamType, uint16_t pid, iOptions* options)
    : cRender(kQueued, name, "aud", options, streamType, pid,
              kDefaultPtsPerFrame, (dynamic_cast<cRender::cOptions*>(options))->mIsLive ? kLiveMaxFrames : kFileMaxFrames,

              // getFrame lambda
              [&]() noexcept {
                return hasMaxFrames() ? reuseBestFrame() : new cAudioFrame();
                },

              // addFrame lambda
              [&](cFrame* frame) noexcept {
                cAudioFrame* audioFrame = dynamic_cast<cAudioFrame*>(frame);
                if (mSampleRate != audioFrame->getSampleRate()) {
                  cLog::log (LOGERROR, fmt::format ("cAudioRender::addFrame sampleRate changed {} to {}",
                                                    mSampleRate, audioFrame->getSampleRate()));
                  mSampleRate = audioFrame->getSampleRate();
                  }

                setPts (frame->getPts(), frame->getPtsDuration(), frame->getStreamPos());
                mSamplesPerFrame = audioFrame->getSamplesPerFrame();
                mFrameInfo = audioFrame->getInfoString();
                audioFrame->calcPower();

                cRender::addFrame (frame);

                if (!mPlayer) {
                  mPlayer = new cPlayer (*this, mSampleRate, getPid(), (dynamic_cast<cAudioRender::cOptions*>(mOptions)->mHasAudio));
                  mPlayer->startPlayPts (audioFrame->getPts());
                  }
                }),
      mSampleRate(48000), mSamplesPerFrame(1024) {

  mDecoder = new cFFmpegAudioDecoder ((streamType == 17) ? eAudioFrameType::eAacLatm : eAudioFrameType::eMp3);
  }
//}}}

//{{{
cAudioFrame* cAudioRender::getAudioFrameAtPts (int64_t pts) {

  return (mFramesMap.empty() || !getPtsDuration()) ?
    nullptr : dynamic_cast<cAudioFrame*>(getFrameAtPts (pts));
  }
//}}}
//{{{
cAudioFrame* cAudioRender::getAudioFrameAtOrAfterPts (int64_t pts) {

  return (mFramesMap.empty() || !getPtsDuration()) ?
    nullptr : dynamic_cast<cAudioFrame*>(getFrameAtOrAfterPts (pts));
  }
//}}}

// overrides
//{{{
string cAudioRender::getInfoString() const {
  return fmt::format ("aud {} {} {}", cRender::getInfoString(), mDecoder->getInfoString(), mFrameInfo);
  }
//}}}
//{{{
bool cAudioRender::processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize,
                               int64_t pts, int64_t dts, int64_t streamPos, int64_t skipPts) {

  if (!((dynamic_cast<cRender::cOptions*>(mOptions))->mIsLive))
    if (mPlayer)
      while (throttle (mPlayer->getPts()))
        this_thread::sleep_for (1ms);

  return cRender::processPes (pid, pes, pesSize, pts, dts, streamPos, skipPts);
  }
//}}}
//{{{
void cAudioRender::skip (int64_t skipPts) {
  if (mPlayer)
    mPlayer->startPlayPts (getPts() + skipPts);
  }
//}}}
//{{{
void cAudioRender::togglePlay() {
  if (mPlayer)
    mPlayer->togglePlay();
  }
//}}}

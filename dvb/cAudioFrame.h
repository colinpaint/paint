// cAudioFrame.h
//{{{  includes
#pragma once
#include "cRender.h"

#include <cstdint>
#include <array>
#include <algorithm>

#include <math.h>
//}}}

constexpr size_t kMaxAudioChannels = 6;
constexpr size_t kMaxAudioSamplesPerFrame = 2048;

class cAudioFrame : public cFrame {
public:
  cAudioFrame() = default;
  virtual ~cAudioFrame() { releaseResources(); }

  std::string getInfoString() { return fmt::format ("pesSize:{:6} {}x{}:{}k",
                                                    mPesSize, mNumChannels, mSamplesPerFrame, mSampleRate/1000); }

  virtual void releaseResources() final {}
  //{{{
  void calcPower() {

    // init
    for (size_t channel = 0; channel < mNumChannels; channel++) {
      mPeakValues[channel] = 0.f;
      mPowerValues[channel] = 0.f;
      }

    // acumulate
    float* samplePtr = mSamples.data();
    for (size_t sample = 0; sample < mSamplesPerFrame; sample++) {
      for (size_t channel = 0; channel < mNumChannels; channel++) {
        float value = *samplePtr++;
        mPeakValues[channel] = std::max (abs(mPeakValues[channel]), value);
        mPowerValues[channel] += value * value;
        }
      }

    // normalise
    for (size_t channel = 0; channel < mNumChannels; channel++)
      mPowerValues[channel] = sqrtf (mPowerValues[channel] / mSamplesPerFrame);
    }
  //}}}

  // vars
  size_t mNumChannels = kMaxAudioChannels;
  size_t mSamplesPerFrame = kMaxAudioSamplesPerFrame;
  uint32_t mSampleRate = 48000;

  std::array <float, kMaxAudioChannels> mPeakValues = {0.f};
  std::array <float, kMaxAudioChannels> mPowerValues = {0.f};
  std::array <float, kMaxAudioChannels * kMaxAudioSamplesPerFrame> mSamples = {0.f};
  };

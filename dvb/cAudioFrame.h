// cAudioFrame.h
//{{{  includes
#pragma once
#include "cRender.h"

#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include <math.h>
//}}}

class cAudioFrame : public cFrame {
public:
  cAudioFrame() {}
  //{{{
  ~cAudioFrame() {
    free (mSamples);
    }
  //}}}

  //{{{
  void setSamples (float* samples) {

    reset();

    mSamples = samples;

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
        mPeakValues[channel] = std::max (abs(mPeakValues[channel]), value);
        mPowerValues[channel] += value * value;
        }
      }

    for (size_t channel = 0; channel < mNumChannels; channel++)
      mPowerValues[channel] = sqrtf (mPowerValues[channel] / mSamplesPerFrame);
    }
  //}}}
  //{{{
  virtual void reset() final {
    free (mSamples);
    mSamples = nullptr;
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

  // vars
  size_t mNumChannels;
  size_t mSamplesPerFrame;
  uint32_t mSampleRate;

  float* mSamples = nullptr;

  std::array <float, 6> mPeakValues = {0.f};
  std::array <float, 6> mPowerValues = {0.f};
  };

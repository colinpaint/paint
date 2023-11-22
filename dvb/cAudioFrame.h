// cAudioFrame.h
//{{{  includes
#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <algorithm>
#include <math.h>

#include "cFrame.h"
//}}}
constexpr size_t kMaxAudioChannels = 6;
constexpr size_t kMaxAudioSamplesPerFrame = 2048;

class cAudioFrame : public cFrame {
public:
  cAudioFrame() {}
  virtual ~cAudioFrame() {
    releaseResources();
    }

  //{{{
  std::string getInfoString() {

    std::string info = fmt::format ("{}x{}:{}k pesSize:{:3}",
                                    mNumChannels, mSamplesPerFrame, mSampleRate/1000, mPesSize);
    if (!mTimes.empty()) {
      info += " took";
      for (auto time : mTimes)
        info += fmt::format (" {}us", time);
      }

    return info;
    }
  //}}}

  float getSimplePower() const { return mSimplePower; }
  //{{{
  void calcPower() {

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
    for (size_t channel = 0; channel < mNumChannels; channel++) {
      mPowerValues[channel] = sqrtf (mPowerValues[channel] / mSamplesPerFrame);
      mSimplePower += mPowerValues[channel];
      }

    mSimplePower /= mNumChannels;
    }
  //}}}

  void addTime (int64_t time) { mTimes.push_back (time); }

  virtual void releaseResources() final {
    mTimes.clear();
    }

  // vars
  size_t mNumChannels = kMaxAudioChannels;
  size_t mSamplesPerFrame = kMaxAudioSamplesPerFrame;
  uint32_t mSampleRate = 48000;

  float mSimplePower = 0.f;
  std::array <float, kMaxAudioChannels> mPeakValues = {0.f};
  std::array <float, kMaxAudioChannels> mPowerValues = {0.f};
  std::array <float, kMaxAudioChannels * kMaxAudioSamplesPerFrame> mSamples = {0.f};

  std::vector <int64_t> mTimes;
  };

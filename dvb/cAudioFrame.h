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

  size_t getNumChannels() const  { return mNumChannels; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  uint32_t getSampleRate() const { return mSampleRate; }
  //{{{
  std::string getInfoString() {

    std::string info = fmt::format ("{}x{}:{}k pesSize:{:3}",
                                    mNumChannels, mSamplesPerFrame, mSampleRate/1000, getPesSize());
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

    // crude limit of channels for 5.1
    mSimplePower /= std::max (mNumChannels,size_t(3));
    }
  //}}}

  void setNumChannels (size_t numChannels) { mNumChannels = numChannels; }
  void setSamplesPerFrame (size_t samplesPerFrame) { mSamplesPerFrame = samplesPerFrame; }
  void setSampleRate (uint32_t sampleRate) { mSampleRate = sampleRate; }

  void addTime (int64_t time) { mTimes.push_back (time); }

  virtual void releaseResources() final {
    mTimes.clear();
    }

  // vars
  float mSimplePower = 0.f;
  std::array <float, kMaxAudioChannels> mPeakValues = {0.f};
  std::array <float, kMaxAudioChannels> mPowerValues = {0.f};
  std::array <float, kMaxAudioChannels * kMaxAudioSamplesPerFrame> mSamples = {0.f};

  std::vector <int64_t> mTimes;

private:
  size_t mNumChannels = kMaxAudioChannels;
  size_t mSamplesPerFrame = kMaxAudioSamplesPerFrame;
  uint32_t mSampleRate = 48000;
  };

// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cRender.h"
#include "../common/cMiniLog.h"

class cAudioFrame;
class cAudioDecoder;
class cAudioPlayer;
//}}}

constexpr size_t kAudioFrameMapSize = 16;
enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm };

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string& name, uint8_t streamType, uint16_t decoderMask);
  virtual ~cAudioRender();

  // gets
  size_t getNumChannels() const { return mNumChannels; }
  uint32_t getSampleRate() const { return mSampleRate; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  int64_t getPtsDuration() const { return mPtsDuration; }

  // play
  bool isPlaying() const;
  int64_t getPlayerPts() const;

  void togglePlaying();
  void startPlayerPts (int64_t pts);

  // find
  cAudioFrame* getAudioFrameFromPts (int64_t pts);

  // callbacks
  virtual cFrame* getFrame() final;
  virtual void addFrame (cFrame* frame) final;

  virtual std::string getInfoString() const final;
  virtual bool processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) final;

private:
  // vars
  size_t mNumChannels;
  uint32_t mSampleRate;
  size_t mSamplesPerFrame;
  int64_t mPtsDuration;

  std::string mFrameInfo;

  cAudioPlayer& mPlayer;
  int mPlayerFrames;
  };

// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <thread>

#include "cRender.h"

class cAudioFrame;
class cAudioDecoder;
class cPlayer;
//}}}

//enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm };

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string& name, uint8_t streamType, uint16_t pid, bool live, bool hasAudio);
  virtual ~cAudioRender() = default;

  // gets
  size_t getSampleRate() const { return mSampleRate; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }

  cPlayer* getPlayer() { return mPlayer; }

  // find
  cAudioFrame* getAudioFrameAtPts (int64_t pts);
  cAudioFrame* getAudioFrameAtOrAfterPts (int64_t pts);

  // overrides
  virtual std::string getInfoString() const final;
  virtual bool processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) final;

private:
  uint32_t mSampleRate;
  size_t mSamplesPerFrame;
  const bool mLive;
  const bool mHasAudio;
  cPlayer* mPlayer = nullptr;

  std::string mFrameInfo;
  };

// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>
#include <thread>

#include "cRender.h"

class cAudioFrame;
class cAudioDecoder;
class cAudioPlayer;
//}}}

class cAudioRender : public cRender {
public:
  cAudioRender (bool queue, size_t maxFrames, bool playerHasAudio, uint8_t streamType, uint16_t pid);
  virtual ~cAudioRender() = default;

  // gets
  size_t getSampleRate() const { return mSampleRate; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }

  cAudioPlayer* getAudioPlayer() { return mAudioPlayer; }

  // find
  cAudioFrame* getAudioFrameAtPts (int64_t pts);
  cAudioFrame* getAudioFrameAtOrAfterPts (int64_t pts);

  virtual bool throttle (int64_t playPts) final { return cRender::throttle (playPts); }

  // overrides
  virtual std::string getInfoString() const final;
  virtual bool throttle() final;

private:
  uint32_t mSampleRate;
  size_t mSamplesPerFrame;
  const bool mPlayerHasAudio = false;

  cAudioPlayer* mAudioPlayer = nullptr;

  std::string mFrameInfo;
  };

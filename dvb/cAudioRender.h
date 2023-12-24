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

class cAudioRender : public cRender {
public:
  //{{{
  class cOptions {
  public:
    virtual ~cOptions() = default;

    bool mHasAudio = true;
    };
  //}}}

  cAudioRender (const std::string& name, uint8_t streamType, uint16_t pid, iOptions* options);
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
  virtual bool processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize,
                           int64_t pts, int64_t dts, int64_t streamPos, bool skip) final;
  virtual void togglePlay() final;
  virtual void skip (int64_t skipPts) final;

private:
  uint32_t mSampleRate;
  size_t mSamplesPerFrame;

  cPlayer* mPlayer = nullptr;

  std::string mFrameInfo;
  };

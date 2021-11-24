// cAudioRender.h
//{{{  includes
#pragma once

#include <cstdint>
#include <string>

#include "cRender.h"
#include "../utils/cMiniLog.h"

class cAudioFrame;
class cAudioDecoder;
class cAudioPlayer;
//}}}

enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm };

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string& name, uint8_t streamTypeId, uint16_t decoderMask);
  virtual ~cAudioRender();

  // gets
  size_t getNumChannels() const { return mNumChannels; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  uint32_t getSampleRate() const { return mSampleRate; }

  bool isPlaying() const;
  int64_t getPlayPts() const;

  // play
  void togglePlaying();
  void setPlayPts (int64_t pts);

  // find
  cAudioFrame* findFrame (int64_t pts) const;
  cAudioFrame* findPlayFrame() const;

  // virtuals
  virtual std::string getInfo() const final;
  virtual void addFrame (cFrame* frame) final;

private:
  // vars
  size_t mNumChannels;
  size_t mSamplesPerFrame = 0;
  uint32_t mSampleRate = 0;

  cAudioPlayer* mPlayer = nullptr;
  };

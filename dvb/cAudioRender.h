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
  cAudioRender (const std::string name, uint8_t streamType, uint16_t decoderMask);
  virtual ~cAudioRender();

  // gets
  size_t getNumChannels() const { return mNumChannels; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  uint32_t getSampleRate() const { return mSampleRate; }

  bool getPlaying() const;
  int64_t getPlayPts() const;
  virtual std::string getInfoString() const final;

  // play
  void togglePlaying();
  void setPlayPts (int64_t pts);

  // find
  cAudioFrame* findFrame (int64_t pts) const;
  cAudioFrame* findPlayFrame() const;

  void addFrame (cAudioFrame* frame);
  virtual void addFrame (cFrame* frame) final { (void)frame; }
  virtual void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) final;

private:
  // vars
  size_t mNumChannels;
  size_t mSamplesPerFrame = 0;
  uint32_t mSampleRate = 0;

  size_t mMaxMapSize;
  std::map <int64_t, cAudioFrame*> mFrames;

  cAudioPlayer* mPlayer = nullptr;
  };

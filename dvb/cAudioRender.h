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
  cAudioRender (const std::string name);
  virtual ~cAudioRender();

  // gets
  eAudioFrameType getFrameType() const { return mFrameType; }
  size_t getNumChannels() const { return mNumChannels; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  uint32_t getSampleRate() const { return mSampleRate; }

  bool getPlaying() const;
  int64_t getPlayPts() const;
  std::string getInfoString() const;

  // play
  void togglePlaying();
  void setPlayPts (int64_t pts);

  // find
  cAudioFrame* findPlayFrame() const;

  void processPes (uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts);

private:
  cAudioFrame* findFrame (int64_t pts) const;
  void addFrame (cAudioFrame* frame);

  // vars
  cAudioDecoder* mDecoder = nullptr;

  eAudioFrameType mFrameType;
  size_t mNumChannels;
  size_t mSamplesPerFrame = 0;
  uint32_t mSampleRate = 0;

  size_t mMaxMapSize;
  std::map <int64_t, cAudioFrame*> mFrames;

  cAudioPlayer* mPlayer = nullptr;
  };

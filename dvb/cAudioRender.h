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

enum class eAudioFrameType { eUnknown, eId3Tag, eWav, eMp3, eAacAdts, eAacLatm };

class cAudioRender : public cRender {
public:
  cAudioRender (const std::string& name, uint8_t streamType, bool realTime);
  virtual ~cAudioRender();

  // gets
  size_t getSampleRate() const { return mSampleRate; }
  size_t getSamplesPerFrame() const { return mSamplesPerFrame; }
  int64_t getPtsDuration() const { return mPtsDuration; }

  cPlayer& getPlayer() { return *mPlayer; }

  // find
  cAudioFrame* findAudioFrameFromPts (int64_t pts);

  // callbacks
  cFrame* getFrame();
  void addFrame (cFrame* frame);

  // overrides
  virtual std::string getInfoString() const final;
  virtual bool processPes (uint16_t pid, uint8_t* pes, uint32_t pesSize, int64_t pts, int64_t dts, bool skip) final;

private:
  cPlayer* mPlayer = nullptr;

  // vars
  uint32_t mSampleRate;
  size_t mSamplesPerFrame;
  int64_t mPtsDuration;

  std::string mFrameInfo;
  };
